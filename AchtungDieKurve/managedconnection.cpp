#include "managedconnection.h"

void GameSettings::sendGameSettings(ManagedConnection *connection, int EPOLL)
{
    QByteArray arr;
    QDataStream stream(&arr, QIODevice::WriteOnly);

    stream << BOARD_SIZE;
    stream << WALL_THICKNESS;
    stream << PLAYER_SIZE;
    stream << LINE_SIZE;
    stream << NO_DRAW_MIN;
    stream << NO_DRAW_MAX;
    stream << COLLISION_DRAW_FIRST_IGNORE;
    stream << DATA_SEND_TIMEOUT;
    stream << WIN_TIME;
    stream << GRACE_TIME;
    stream << FREEZE_TIME;
    stream << TICK_RATE;
    stream << FREEZE_FONT_SIZE;
    stream << WIN_FONT_SIZE;
    stream << NAME_FONT_SIZE;

    stream << SPEED;
    stream << ANGLE_SPEED;
    stream << NO_DRAW_CHANCE;

    connection->sendData(MessageType::gameStart, arr, EPOLL);
}

void GameSettings::loadGameSettings(QByteArray arr)
{
    QDataStream stream(&arr, QIODevice::ReadOnly);

    stream >> BOARD_SIZE;
    stream >> WALL_THICKNESS;
    stream >> PLAYER_SIZE;
    stream >> LINE_SIZE;
    stream >> NO_DRAW_MIN;
    stream >> NO_DRAW_MAX;
    stream >> COLLISION_DRAW_FIRST_IGNORE;
    stream >> DATA_SEND_TIMEOUT;
    stream >> WIN_TIME;
    stream >> GRACE_TIME;
    stream >> FREEZE_TIME;
    stream >> TICK_RATE;
    stream >> FREEZE_FONT_SIZE;
    stream >> WIN_FONT_SIZE;
    stream >> NAME_FONT_SIZE;

    stream >> SPEED;
    stream >> ANGLE_SPEED;
    stream >> NO_DRAW_CHANCE;
}

bool ManagedConnection::connect(quint32 _address, quint16 _port)
{
    close();
    
    address = _address;
    udpPort = port = _port;

    tcpSocket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if (tcpSocket == -1)
    {
        error("Unable to start the client: ");
        return false;
    }

    if (fcntl(tcpSocket, F_SETFL, O_NONBLOCK) == -1)
    {
        error("Unable to start the server in non-blocking mode: ");
        return false;
    }

    sockaddr_in sockAddress;
    memset(&sockAddress, 0, sizeof(sockaddr_in));
    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = port;
    sockAddress.sin_addr.s_addr = address;

    ::connect(tcpSocket, reinterpret_cast<struct sockaddr*>(&sockAddress), sizeof(sockaddr_in));
    if (errno != EINPROGRESS)
    {
        error("Unable to connect to the selected address and port: ");
        return false;
    }
    return true;
}

bool ManagedConnection::readData(unsigned char& msgType, QByteArray& msg)
{
    //if (socket->state() != QTcpSocket::ConnectedState)
    if (!bConnected)
        return false;

    int ret = -1;
    while (ret != 0)
    {
        char dat[250];
        ret = recv(tcpSocket, dat, 250, MSG_DONTWAIT);
        if (ret == -1)
        {
            if (errno != EAGAIN)
            {
                error("Error during data read: ");
                return false;
            }
            break;
        }
        else
            data.append(dat, ret);
    }
    //Przetwarzanie uzwględnia fakt, że wiadomość może dotrzeć niepełna, dlatego wykorzystujemy bufor [data]
    //data    += socket->readAll();

    //Sprawdzamy, czy wiadomość ma wystarczająca długość, żeby zawierać:
    //- typ wiadomości (unsigned char)
    //- rozmiar wiadomości (qint32)
    if (static_cast<unsigned>(data.size()) < sizeof(unsigned char) + sizeof(qint32))
        return false;

    msgType     = static_cast<unsigned char>(data.at(0));

    QByteArray  sizeMsg = data.left(sizeof(qint32) + sizeof(unsigned char)).remove(0, sizeof(unsigned char));
    QDataStream sizeStream(&sizeMsg, QIODevice::ReadOnly);

    qint32          size;
    sizeStream  >>  size;

    //Mamy preambułę wiadomości, więc trzeba teraz sprawdzić, czy cała do nas dotarła
    if (static_cast<unsigned>(data.size()) < sizeof(unsigned char) + sizeof(qint32) + static_cast<unsigned>(size))
        return false;

    //Usuwa preambułę z bufora
    data.remove(0, sizeof(unsigned char) + sizeof(qint32));
    //Zapisuje dane w przekazanej przez referencję zmiennej
    msg = data.left(size);
    //Usuwa dane z bufora
    data.remove(0, size);

    return true;
}

bool ManagedConnection::readDatagram(unsigned char& msgType, QByteArray& msg, quint32* sender, quint16* senderPort)
{
    if (!bConnected || udpSocket == -1)
        return false;

    QByteArray          readMsg;
    //Gdyby się nie zmieściło, to przycięcie wiadomości nie stanowi problemu (zabezpieczone przed crashem). Server po prostu będzie musiał wysyłać te same ponownie
    char                *data = new char[30000];
    sockaddr_in         sockAddress;
    socklen_t           sizeSockAddr    = sizeof(sockaddr_in);
    int receive = recvfrom(udpSocket, data, 30000, MSG_DONTWAIT, reinterpret_cast<struct sockaddr *>(&sockAddress), &sizeSockAddr);
    if (receive == -1)
    {
        if (errno != EAGAIN)
            error("Error during data receive: ");
        delete[] data;
        return false;
    }
    if (sender)
        *sender = sockAddress.sin_addr.s_addr;
    if (senderPort)
        *senderPort = sockAddress.sin_port;
    if (receive)
    {
        //QNetworkDatagram    datagram    = udpSocket->receiveDatagram();

        //QByteArray          readMsg     = datagram.data();
        //QDataStream         stream(&readMsg, QIODevice::ReadOnly);
        unsigned            size;
        readMsg.append(data, receive);

        QDataStream         stream(&readMsg, QIODevice::ReadOnly);
        stream >> msgType;

        char* msgData;
        stream.readBytes(msgData, size);
        msg.append(msgData, static_cast<int>(size));
        delete[] msgData;
    }
    delete[] data;
    return true;
}

bool ManagedConnection::bindUdpSocket(quint16 _port)
{
    if (udpSocket != -1)
    {
        epoll_event udpev;
        memset(&udpev, 0, sizeof(epoll_event));
        if (epoll_ctl(connectionEPOLL, EPOLL_CTL_DEL, udpSocket, &udpev) == -1 && errno != ENOENT)
        {
            error("Unable to remove old socket from a pooling structure: ");
            return false;
        }
        ::close(udpSocket);
    }
    udpSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (udpSocket == -1)
    {
        error("Unable to create a socket: ");
        return false;
    }

    if (fcntl(udpSocket, F_SETFL, O_NONBLOCK) == -1)
    {
        error("Unable to set a socket in a non-blocking mode: ");
        return false;
    }

    udpPort = _port;
    //bool binded = udpSocket->bind(QHostAddress(address), udpPort, QAbstractSocket::ShareAddress);
    sockaddr_in sockAddress;
    memset(&sockAddress, 0, sizeof(sockaddr_in));
    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = udpPort;
    sockAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(udpSocket, reinterpret_cast<struct sockaddr *>(&sockAddress), sizeof(sockaddr_in)) == -1)
    {
        error("Failed to bind UDP socket to port " + std::to_string(_port) + ": ");
        return false;
    }
    if (connectionEPOLL == -1)
        connectionEPOLL = epoll_create(1);
    if (connectionEPOLL == -1)
    {
        error("Unable to create pooling structure: ");
        return false;
    }
    epoll_event ev;
    memset(&ev, 0, sizeof(epoll_event));
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
    ev.data.fd = udpSocket;
    if (epoll_ctl(connectionEPOLL, EPOLL_CTL_ADD, udpSocket, &ev) == -1)
    {
        if (errno == EEXIST && epoll_ctl(connectionEPOLL, EPOLL_CTL_MOD, udpSocket, &ev) != -1)
            return true;

        error("Unable to setup pooling structure: ");
        return false;
    }
    return true;
}

bool ManagedConnection::bindTcpSocket(quint16 _port)
{
    port = _port;
    //bool binded = socket->bind(QHostAddress(address), udpPort, QAbstractSocket::ShareAddress);
    sockaddr_in sockAddress;
    memset(&sockAddress, 0, sizeof(sockaddr_in));
    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = port;
    sockAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(tcpSocket, reinterpret_cast<struct sockaddr *>(&sockAddress), sizeof(sockaddr_in)) == -1)
    {
        error("Unable to bind the server to the selected address and port: ");
        return false;
    }
    return true;
    //Tutaj nie dodamy do EPOLLA, bo bindowanie TCP dzieje się tylko dla serwera i ominęlibyśmy klienta
    //Trzeba to zrobić w innych funkcjach
}

void ManagedConnection::sendDatagram(const unsigned char msgType, QByteArray msg, int EPOLL, quint32 destAddr, quint16 destPort)
{
    if (!bConnected || udpSocket == -1)
        return;

    if (EPOLL == -1)
        EPOLL = connectionEPOLL;

    if (!destPort)
        destPort = udpPort;

    if (!destAddr)
        destAddr = address;

    QByteArray  sendMsg;
    QDataStream stream(&sendMsg, QIODevice::WriteOnly);

    stream  << msgType;
    stream.writeBytes(msg, static_cast<unsigned>(msg.size()));
    auto same = std::find_if(datagrams.begin(), datagrams.end(), [&sendMsg](std::pair<sockaddr_in, QByteArray>& p) { return p.second == sendMsg; });
    if (same != datagrams.end())
        return;

#ifdef QT_Socket
    udpSocket->writeDatagram(sendMsg, addr, destPort);
#else
    sockaddr_in sockAddress;
    memset(&sockAddress, 0, sizeof(sockaddr_in));
    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = destPort;
    sockAddress.sin_addr.s_addr = destAddr;
    //if (sendto(udpSocket, sendMsg, sendMsg.size(), MSG_DONTWAIT, reinterpret_cast<struct sockaddr*> (&sockAddress), sizeof(sockaddr_in)) == -1 && errno != EAGAIN)
        //error("Unable to send datagram: ");
    datagrams.push_back(std::pair<sockaddr_in, QByteArray>(sockAddress, sendMsg));
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLOUT;
    ev.data.fd = udpSocket;
    if (epoll_ctl(EPOLL, EPOLL_CTL_MOD, udpSocket, &ev) == -1)
        error("Unable to setup pooling structure: ");
#endif
}


void ManagedConnection::sendQueuedDatagram(int EPOLL)
{
    if (!bConnected || datagrams.empty())
        return;
    if (EPOLL == -1)
        EPOLL = connectionEPOLL;

    for (std::pair<sockaddr_in, QByteArray> &datagram : datagrams)
    {
        int sent = 0;
        if (sendto(udpSocket, datagram.second, datagram.second.size(), MSG_DONTWAIT, reinterpret_cast<struct sockaddr*> (&(datagram.first)), sizeof(sockaddr_in)) == -1)
        {
            if (errno == EAGAIN)
                break;
            else
                error("Unable to send datagram: ");
        }
    }
    datagrams.clear();
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
    ev.data.fd = udpSocket;
    if (epoll_ctl(EPOLL, EPOLL_CTL_MOD, udpSocket, &ev) == -1)
        error("Unable to setup pooling structure: ");
}

void ManagedConnection::sendData(const unsigned char msgType, QByteArray msg, int EPOLL)
{
    //if (socket->state() != QTcpSocket::ConnectedState)
    if (!bConnected)
        return;
    if (EPOLL == -1)
        EPOLL = connectionEPOLL;

    QByteArray sendMsg;
    QDataStream stream(&sendMsg, QIODevice::WriteOnly);

    stream << msgType;
    stream.writeBytes(msg, static_cast<unsigned>(msg.size()));

    //socket->write(sendMsg);
    messages.append(sendMsg);

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLOUT;
    ev.data.fd = tcpSocket;
    if (epoll_ctl(EPOLL, EPOLL_CTL_MOD, tcpSocket, &ev) == -1)
        error("Unable to setup pooling structure: ");
}

void ManagedConnection::sendQueuedData(int EPOLL)
{
    if (!bConnected || !messages.size())
        return;
    if (EPOLL == -1)
        EPOLL = connectionEPOLL;

    while (messages.size())
    {
        int sendBytes = send(tcpSocket, messages, messages.size(), MSG_DONTWAIT);
        if (sendBytes == 0)
            break;
        if (sendBytes == -1)
        {
            if (errno == EAGAIN)
                break;
            else
                error("Unable to write to a socket stream: ");
        }

        messages.remove(0, sendBytes);
    }

    if (!messages.size())
    {
        epoll_event ev;
        ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
        ev.data.fd = tcpSocket;
        if (epoll_ctl(EPOLL, EPOLL_CTL_MOD, tcpSocket, &ev) == -1)
            error("Unable to setup pooling structure: ");
    }
}

bool ManagedConnection::listen(quint16 _port)
{
    tcpSocket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    udpPort = port = _port;
    if (tcpSocket == -1)
    {
        error("Unable to start the server: ");
        return false;
    }

    if (fcntl(tcpSocket, F_SETFL, O_NONBLOCK) == -1)
    {
        error("Unable to start the server in non-blocking mode: ");
        return false;
    }

    if (!bindTcpSocket(port))
    {
        return false;
    }

    bool ret = (::listen(tcpSocket, 10) != -1);
    if (!ret)
    {
        error("Unable to listen to the selected address and port: ");
        return false;
    }

    if (connectionEPOLL == -1)
        connectionEPOLL = epoll_create(1);
    if (connectionEPOLL == -1)
    {
        error("Unable to create pooling structure for the server: ");
        return false;
    }
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
    ev.data.fd = tcpSocket;
    if (epoll_ctl(connectionEPOLL, EPOLL_CTL_ADD, tcpSocket, &ev) == -1)
    {
        if (errno == EEXIST)
        {
            if (epoll_ctl(connectionEPOLL, EPOLL_CTL_MOD, tcpSocket, &ev) == -1)
                error("Unable to setup pooling structure for the server: ");
        }
        else
            error("Unable to setup pooling structure for the server: ");
        return false;
    }
    bindUdpSocket(port);
    bConnected = true;
    return true;
}

void ManagedConnection::reset()
{
    data.clear();
    datagrams.clear();
    messages.clear();
    //while (udpSocket->hasPendingDatagrams())
        //udpSocket->receiveDatagram();
}

void ManagedConnection::close()
{
    bConnected = false;
    reset();

    if (connectionEPOLL != -1)
        ::close(connectionEPOLL);

    if (tcpSocket != -1)
        ::close(tcpSocket);

    if (udpSocket != -1)
        ::close(udpSocket);

    //if (tcpSocket)
        //tcpSocket->close();
    tcpSocket = connectionEPOLL = udpSocket = -1;
}

Player* ManagedConnection::getPlayer(std::vector<Player>& players)
{
    if (!bConnected || playerName.empty())
        return nullptr;

    auto it = std::find(players.begin(), players.end(), playerName);

    if (it == players.end())
        return nullptr;

    return &*it;
}

void ManagedConnection::error(std::string title, bool bCritical)
{
    if (bCritical)
    {
        close();
        QMessageBox::critical(NULL, "Server",
                          (title
                          + strerror(errno)).c_str() /*tcpSocket.errorString()*/);
    }
    else
        QMessageBox::information(NULL, "Server",
                          (title
                          + strerror(errno)).c_str() /*tcpSocket.errorString()*/);
}

bool ManagedConnection::connected()
{
    bConnected = true;

    if (connectionEPOLL == -1)
        connectionEPOLL = epoll_create(1);
    if (connectionEPOLL == -1)
    {
        error("Unable to create pooling structure for the client: ");
        return false;
    }

    epoll_event ev;
    memset(&ev, 0, sizeof(epoll_event));
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP;
    ev.data.fd = tcpSocket;
    if (epoll_ctl(connectionEPOLL, EPOLL_CTL_ADD, tcpSocket, &ev) == -1)
    {
        if (errno == EEXIST && epoll_ctl(connectionEPOLL, EPOLL_CTL_MOD, tcpSocket, &ev) != -1)
            return true;
        return false;
    }
    return true;
}
