#include "gamelogic.h"

void Server::acceptConnection()
{
    //while (server.hasPendingConnections())
    sockaddr_in sockAddress;
    memset(&sockAddress, 0, sizeof(sockAddress));
    socklen_t size = sizeof(sockaddr_in);
    int accepted = accept(server.tcpSocket, reinterpret_cast<struct sockaddr*>(&sockAddress), &size);
    if (accepted == -1)
    {
        if (errno != EAGAIN)
            server.error("Failed to accept an incoming connection: ", false);
        return;
    }
    if (accepted != 0)
    {
        if (fcntl(accepted, F_SETFL, O_NONBLOCK) == -1)
        {
            server.error("Unable to set a socket in a non-blocking mode: ", false);
            ::close(accepted);
            return;
        }
        clients.emplace_back(accepted, static_cast<quint32>(sockAddress.sin_addr.s_addr), sockAddress.sin_port);
        //clients.emplace_back(server.nextPendingConnection());
        struct epoll_event ev;
        memset(&ev, 0, sizeof(epoll_event));
        ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP;
        ev.data.fd = accepted;
        if (epoll_ctl(server.connectionEPOLL, EPOLL_CTL_ADD, accepted, &ev) == -1)
        {
            if (!(errno == EEXIST && epoll_ctl(server.connectionEPOLL, EPOLL_CTL_MOD, accepted, &ev) != -1))
                server.error("Unable to setup pooling structure: ");
        }
        ManagedConnection& connection = clients.back();
        //Unikalny port
        for (unsigned short freePort = NetworkSettings::UDP_PORT_START; freePort != NetworkSettings::UDP_PORT_START + static_cast<unsigned short>(clients.size()) + 1u; ++freePort)
        {
            bool bFree = true;
            quint16 htoned = htons(freePort);
            for (ManagedConnection &client : clients)
            {
                if (htoned == client.udpPort)
                {
                    bFree = false;
                    break;
                }
            }
            if (bFree)
            {
                connection.udpPort = htoned;
                break;
            }
        }

        //Unikalny token
        bool bRollingToken = true;
        quint32 token;
        while (bRollingToken)
        {
            bRollingToken = false;

            token = QRandomGenerator::global()->generate();

            //By?? mo??e akurat roz????czony gracz ma taki sam token
            for (Player &player : data->players)
                if (token == player.token)
                {
                    bRollingToken = true;
                    break;
                }
        }
        connection.token = token;
        QByteArray acceptMsg;
        QDataStream acceptStream(&acceptMsg, QIODevice::WriteOnly);

        acceptStream << connection.token << connection.udpPort; //port do komunikacji UDP

        connection.sendData(MessageType::accepted, acceptMsg, server.connectionEPOLL);
    }
}

void Server::pool()
{
    if (!server.bConnected)
        return;
    struct epoll_event events[20];
    memset(&events, 0, sizeof(epoll_event)*20);
    int i = epoll_wait(server.connectionEPOLL, events, 20, 0);
    if (i == -1)
        return;
    for (;i != 0; )
    {
        int con = events[--i].data.fd;
        if ((events[i].events & EPOLLERR) && (errno != EAGAIN))
        {
            terminateConnection(con);
            //QMessageBox::critical(NULL, "Error", "Error occured on a client socket: ");
        }
        if (events[i].events & EPOLLRDHUP)
        {
            auto connection = std::find_if(clients.begin(), clients.end(), [con](ManagedConnection& p) { return p.tcpSocket == con; });
            //Musimy najpierw nawi??za?? po????czenie (bConnected), ??eby nie blokowa?? klient??w, kt??rzy dopiero chc?? do????czy??
            if (connection != clients.end() && connection->bConnected)
                terminateConnection(con);
            //QMessageBox::critical(NULL, "Error", "Connection lost");
        }
        if (events[i].events & EPOLLIN)
        {
            if (con == server.tcpSocket) //Nas??uchuj??cy socket
                acceptConnection();
            else if (con == server.udpSocket && data->bRunning)
                logic->readDatagramsServer();
            else
            {
                auto connection = std::find_if(clients.begin(), clients.end(), [con](ManagedConnection& p) { return p.tcpSocket == con; });
                if (connection != clients.end())
                    readData(&*connection);                //chyba istnieje lepszy spos??b na t?? konwersj??
            }
        }
        if (events[i].events & EPOLLOUT)
        {
            auto connection = std::find_if(clients.begin(), clients.end(), [con](ManagedConnection& p) { return p.tcpSocket == con; });
            if (connection != clients.end())
                connection->sendQueuedData(server.connectionEPOLL);

            if (con == server.udpSocket) server.sendQueuedDatagram();
        }
    }
}

//TCP
void Server::readData(ManagedConnection *connection)
{
    QByteArray msg;
    unsigned char messType;
    while (connection->readData(messType, msg))
    {
        QDataStream stream(&msg, QIODevice::ReadOnly);
        if (messType == MessageType::ping)
            connection->sendData(MessageType::ping, "", server.connectionEPOLL);
        else if (messType == MessageType::name)
        {
            connection->bConnected = true;
            quint32 token = 0u;
            stream >> token;
            char *name;
            unsigned size;
            stream.readBytes(name, size);

            Player* player = data->findPlayer(name);
            connection->playerName = name;

            QByteArray msgSend;
            QDataStream sendStream(&msgSend, QIODevice::WriteOnly);
            if (player)
            {
                if (player->token != token)
                {
                    sendStream.writeBytes("\x01", 2u);
                    connection->sendData(MessageType::name, msgSend, server.connectionEPOLL);
                }
                else
                {
                    sendStream.writeBytes("\x04", 2u);
                    connection->token = token;

                    connection->sendData(MessageType::name, msgSend, server.connectionEPOLL);
                    sendStartGameMessage(*connection);
                }
            }
            else
            {
                //Dodanie nowego gracza i po????czenie ManagedConnection z nim (synchronizacja nazwy i token??w)
                data->players.emplace_back(name);
                connection->playerName = name;
                data->players.back().token = connection->token;
                if (data->players.size() < 2)
                {
                    sendStream.writeBytes("\x02", 2u);
                    connection->sendData(MessageType::name, msgSend, server.connectionEPOLL);
                }
                else
                {
                    for (ManagedConnection& con : clients)
                    {
                        QByteArray msgS;
                        QDataStream sendS(&msgS, QIODevice::WriteOnly);
                        sendS.writeBytes("\x03", 2u);
                        con.sendData(MessageType::name, msgS, server.connectionEPOLL);
                    }
                    startGame();
                }
            }
            delete[] name;
        }
        else break;
        connection->lastReceived.start();
        msg.clear();
    }
}

//UDP
void GameLogic::readDatagramsServer()
{
    QByteArray msg;
    unsigned char msgType;

    quint32 sender;
    unsigned short senderPort;
    while (server->server.readDatagram(msgType, msg, &sender, &senderPort))
    {
        QDataStream stream(&msg, QIODevice::ReadOnly);
        if (msgType == MessageType::resendPos)
        {
            std::vector<ManagedConnection::ResendData> resend;
            while (true)
            {
                QByteArray unit;
                unsigned size;
                char *datagramData;
                if (stream.atEnd())
                    break;
                stream.readBytes(datagramData, size);
                unit.append(datagramData, static_cast<int>(size));

                QDataStream unitStream(&unit, QIODevice::ReadOnly);
                char* playerName;
                unitStream.readBytes(playerName, size);

                Player *player = data->findPlayer(playerName);
                if (!player)
                    continue;
                resend.emplace_back(player);
                while (true)
                {
                    if (unitStream.atEnd())
                        break;

                    unsigned res;
                    unitStream >> res;
                    resend.back().confirm.push_back(res);
                }
                delete[] datagramData;
                delete[] playerName;
            }
            QByteArray resendMsg = buildRepositionMessage(resend);
            server->server.sendDatagram(MessageType::sendPos, resendMsg, -1, sender, senderPort);
        }
        else if (msgType == MessageType::sendInput)
        {
            QByteArray unit;
            unsigned size;

            char *playerName;
            stream.readBytes(playerName, size);
            Player * player = data->findPlayer(playerName);
            if (player)
                stream >> player->input;
            delete[] playerName;
        }
        msg.clear();
    }
}

void Server::startGame()
{
    if (data->bRunning)
        return;

    statusLabel->setStyleSheet("QLabel { color : rgb(30,30,30); }");
    statusLabel->setText("Waiting for players");

    if (!clients.size())
        return;

    qint64 tickStart = QDateTime::currentMSecsSinceEpoch();

    //Je??eli uda si?? odpali?? gr??, to wysy??a odpowiednie wiadomo??ci do wszystkich po????czonych klient??w
    if (logic->startGame(tickStart))
        for (ManagedConnection &client : clients)
            sendStartGameMessage(client);
}

void Server::sendStartGameMessage(ManagedConnection &client)
{
    //Gracz mo??e si?? pechowo po????czy?? w trakcie startowania gry, wi??c nie b??dziemy kazali mu ogl??da?? gry, w kt??rej nie uczestniczy.
    //W sumie tylko dlatego, ??e napisali??my w opisie projektu, ??e w tej sytuacji dostanie wiadomo????, ??e musi poczeka?? na nast??pn?? rund??, bo ogl??danie gry i czekanie na now?? rund?? brzmi lepiej
    //Istnieje chyba wyj??cie z tej sytuacji - po????czenie obu. Ogl??danie gry z napisem na g??rze, ??e trzeba poczeka?? na now?? rund??
    if (!(client.getPlayer(data->players)))
        return;
    QByteArray  msg, ticks;
    QDataStream stream(&msg, QIODevice::WriteOnly),
        ticksStream(&ticks, QIODevice::WriteOnly);

    stream << static_cast<unsigned>(data->players.size());
    for (Player& player : data->players)
    {
        stream << player.color << player.bDead;
        stream.writeBytes(player.name.c_str(), static_cast<uint>(player.name.size()));
    }
    ticksStream << logic->startTime << logic->endTime << logic->moveIndex.data << logic->moveIndex.compare << data->bWon;
    ticksStream.writeBytes(data->winner.c_str(), data->winner.size());

    QByteArray ba;
    QBuffer buff(&ba);
    buff.open(QIODevice::WriteOnly);
    data->boardLocal.save(&buff, "BMP");
    ticksStream.writeBytes(ba, static_cast<uint>(ba.size()));
    GameSettings::sendGameSettings(&client, server.connectionEPOLL);

    client.sendData(MessageType::playerSync, msg, server.connectionEPOLL);
    client.sendData(MessageType::tickSync, ticks, server.connectionEPOLL);
}

void GameLogic::sendPosition()
{
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);

    stream << moveIndex; //obecny tick
    //Tworzymy wiadomo???? z pozycj?? ka??dego gracza w nowym ticku
    for (Player& player : data->players)
    {
        QByteArray playerData;
        QDataStream streamPlayer(&playerData, QIODevice::WriteOnly);
        if (!player.bActive)
            continue;

        streamPlayer.writeBytes(player.name.c_str(), static_cast<uint>(player.name.size()));
        streamPlayer    << static_cast<unsigned>(moveIndex)
                        << player.data[static_cast<unsigned>(moveIndex)];

        stream      .writeBytes(playerData, static_cast<uint>(playerData.size()));
        playerData.clear();
    }

    //Wysy??anie do ka??dego klienta
    for (ManagedConnection& con : server->clients)
        server->server.sendDatagram(MessageType::sendPos, msg, -1, con.address, con.udpPort);
}

QByteArray GameLogic::buildRepositionMessage(std::vector<ManagedConnection::ResendData>& resend)
{
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);

    stream << moveIndex; //obecny tick
    for (ManagedConnection::ResendData& res : resend)
    {
        QByteArray  playerMsg;
        QDataStream playerStream(&playerMsg, QIODevice::WriteOnly);
        Player      *&player = res.player;

        if (!player->bActive)
            continue;

        playerStream.writeBytes(player->name.c_str(), static_cast<uint>(player->name.size()));

        //Wysy??amy ka??d?? pozycj?? o indeksie znajduj??cym si?? w tablicy [res.confirm], kt??ra zosta??a
        //zbudowana na podstawie tablicy [player.confirm] u klienta i przes??ana do serwera
        for (unsigned i : res.confirm)
            playerStream << i << player->data[i];

        stream.writeBytes(playerMsg, static_cast<unsigned>(playerMsg.size()));
    }

    return msg;
}

void Server::terminateConnection(int con)
{
    auto connection = std::find_if(clients.begin(), clients.end(), [con](ManagedConnection& p) { return p.tcpSocket == con; });
    if (connection != clients.end())
    {
        //Dokumentacja m??wi, ??e nie trzeba
        //struct epoll_event ev;
        //if (epoll_ctl(server.connectionEPOLL, connectionEPOLL_CTL_DEL, server.tcpSocket, &ev) == -1 && errno != ENOENT)
            //connection->error("Unable to remove old socket from a pooling structure: ");
        //if (epoll_ctl(server.connectionEPOLL, connectionEPOLL_CTL_DEL, server.udpSocket, &ev) == -1 && errno != ENOENT)
            //connection->error("Unable to remove old socket from a pooling structure: ");
        connection->close();
        clients.erase(connection);
    }
}
