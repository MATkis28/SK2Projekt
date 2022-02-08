#include "gamelogic.h"

//TCP
void Client::pool()
{
    if (!connection.bConnected)
    {
        int error = 0;
        socklen_t len = sizeof (error);
        if (connection.tcpSocket == -1)
        {
            pinger.stop();
            return;
        }
        //To powinno zwracać status, czy połączenie zostało nawiązane, ale nie działa (a przynajmniej nie we wszystkich wypadkach)
        if (getsockopt(connection.tcpSocket, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        {
            if (errno == EAGAIN)
                return;
            connection.error("Could not determine client socket state: ");
        }
        if (error == 0)
            //Dlatego mamy dodatkowe sprawdzenie
            if (!connected())
                return;
    }

    epoll_event events[20];
    memset(&events, 0, sizeof(epoll_event) * 20);
    int i = epoll_wait(connection.connectionEPOLL, events, 20, 0);
    if (i == -1)
        return;
    for (; i != 0; )
    {
        int con = events[--i].data.fd;
        if ((events[i].events & EPOLLERR) && (errno != EAGAIN))
            connection.error("Error occured on a client socket: ");
        if ((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLHUP))
        {
            QMessageBox::information(NULL, "Error", "Could not connect / Connection lost");
            close();
        }
        if (events[i].events & EPOLLIN)
        {
            if (con == connection.tcpSocket) //Nasłuchujący socket
                readData();
            else if (con == connection.udpSocket)
                logic->readDatagramsClient();
        }
        if (events[i].events & EPOLLOUT)
        {
            if (con == connection.tcpSocket)
                connection.sendQueuedData();
            else if (con == connection.udpSocket && data->bRunning)
                connection.sendQueuedDatagram();
        }
    }
}

void Client::readData()
{
    QByteArray msg;
    unsigned char msgType;

    while (connection.readData(msgType, msg))
    {
        QDataStream stream(&msg, QIODevice::ReadOnly);
        if (msgType == MessageType::accepted)
        {
            quint32 token = 0;
            stream >> token >> connection.udpPort; //port do komunikacji UDP
            if (connection.token == 0)
                connection.token = token;

            connection.bindUdpSocket(connection.udpPort);
            connection.connected();

            QByteArray msg;
            QDataStream stream(&msg, QIODevice::WriteOnly);
            stream << connection.token;
            stream.writeBytes(connection.playerName.c_str(), connection.playerName.size());
            connection.sendData(MessageType::name, msg);
            pinger.start();
            timeout.start();
        }
        if (msgType == MessageType::name)
        {
            char            *msgData;
            unsigned        size;

            stream.readBytes(msgData, size);

            if (!strcmp(msgData, "\x01")) //nazwa w użyciu
            {
                close();
                QMessageBox::critical(static_cast<QWidget*>(this->parent()), tr("Error"),
                    tr("Name already in use!"));
                delete[] msgData;
                return;
            }
            else if (!data->bRunning)
            {
                statusLabel->setStyleSheet("QLabel { color : rgb(30,30,30); }");
                if (!strcmp(msgData, "\x02")) //za mało graczy
                {
                    statusLabel->setText("Waiting for players");
                }
                else //czekamy na nową rundę
                {
                    if (!strcmp(msgData, "\x03"))
                        statusLabel->setText("Waiting for the round end");
                    else //reconnect
                        statusLabel->setText("Waiting for the reconnection");
                    timeout.start();
                    pinger.start();
                }
            }
            delete[] msgData;
        }
        else if (msgType == MessageType::gameStart)
        {
            GameSettings::loadGameSettings(msg);
        }
        else if (msgType == MessageType::playerSync)
        {
            data->players.clear();

            unsigned size;
            stream >> size;

            for (unsigned i = 0; i != size; ++i)
            {
                QColor      color;
                bool        bDead;
                char        *name;
                unsigned    len;

                stream >> color >> bDead;
                stream.readBytes(name, len);

                data->players.emplace_back(name);
                data->players.back().color = color;
                data->players.back().bDead = bDead;
                delete[] name;
            }
        }
        //Żeby klienci i serwer wystartowali w tym samym czasie
        //Po dodaniu dopracowaniu algorytmu retransmisji, to nie ma aż tak dużego znaczenia, ale nadal przydaje się
        //Do synchronizacji czasu dla przewidywań
        else if (msgType == MessageType::tickSync)
        {
            qint64      msecsStart, msecsEnd;
            unsigned    moveIndexData;
            int         moveIndexCompare;
            bool        bWon;
            stream  >>  msecsStart >> msecsEnd >> moveIndexData >> moveIndexCompare >> bWon;

            char *winner, *boardData;
            uint size;
            QByteArray board;

            stream.readBytes(winner, size);
            if (!winner)
            {
                winner = new char[1];
                winner[0] = '\0';
            }
            stream.readBytes(boardData, size);
            board.append(boardData, static_cast<int>(size));

            data->bRunning = false;
            logic->startGame(msecsStart, msecsEnd, moveIndexData, moveIndexCompare, bWon, winner, board);
            delete[] winner;
            delete[] boardData;
        }
        else if (msgType == MessageType::gameEnd)
        {
            qint64      msecs;
            stream  >>  msecs;

            char*       winner;
            unsigned    size;
            stream.readBytes(winner, size);

            if (!size)
                data->winner = "";
            else data->winner = winner;

            logic->endGame(msecs);
            delete[] winner;
        }
        else break;
        connection.lastReceived.start();
        msg.clear();
    }
    if (timeout.isActive())
        timeout.start();
}

//UDP
void GameLogic::readDatagramsClient()
{
    if (!client->connection.bConnected)
        return;
    QByteArray msg;
    unsigned char msgType;
    while (client->connection.readDatagram(msgType, msg))
    {
        QDataStream stream(&msg, QIODevice::ReadOnly);
        if (msgType == MessageType::sendPos)
        {
            BoundUnsigned tick(moveIndex.boundary);
            stream >> tick;
            if (!(tick < moveIndex))
            {
                //Dostaliśmy nowy tick, więc ustawiamy wszystkie wartości pomiędzy na potencjalne zgubione pakiety
                for (Player &player : data->players)
                {
                    if (!player.bActive || player.bDead)
                        continue;
                    for (BoundUnsigned i = moveIndex; static_cast<unsigned>(i) != static_cast<unsigned>(tick + 1u); ++i)
                        player.confirm[static_cast<unsigned>(i)] = false;
                }
            }
            moveIndex = tick;
            //Prztwarzanie pozycji graczy
            while (!stream.atEnd())
            {
                QByteArray unit;
                QDataStream unitStream(&unit, QIODevice::ReadOnly);
                char *datagramData;
                unsigned size;
                stream.readBytes(datagramData, size);
                unit.append(datagramData, static_cast<int>(size));

                char *playerName;
                unitStream.readBytes(playerName, size);
                while (!unitStream.atEnd())
                {
                    unsigned index;
                    unitStream >> index;
                    Player::MovementData mov;
                    unitStream >> mov;

                    Player *player = data->findPlayer(playerName);
                    if (player && player->bActive)
                    {
                        player->data[index] = mov;
                        //Potwierdzamy otrzymane dane
                        player->confirm[index] = true;
                        //Do wyświetlania "głowy" gracza
                        if (static_cast<unsigned>(index) == static_cast<unsigned>(moveIndex))
                        {
                            player->pos_x = mov.pos_x;
                            player->pos_y = mov.pos_y;
                            player->angle = mov.angle;
                        }
                    }
                }
                delete[] playerName;
                delete[] datagramData;
                unit.clear();
            }
        }
        msg.clear();
    }
}

void Client::sendInput()
{
    if (connection.udpSocket == -1)
        return;
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);
    Player *player = connection.getPlayer(data->players);
    if (player)
    {
        stream.writeBytes(player->name.c_str(), static_cast<uint>(player->name.size()));
        stream << player->input;
        connection.sendDatagram(MessageType::sendInput, msg, connection.connectionEPOLL, connection.address, connection.port);
    }
}

void GameLogic::askForResendPosition()
{
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);

    for (Player &player : data->players)
    {
        if (player.bDead || !player.bActive)
            continue;

        QByteArray resendMsg;
        QDataStream resendStream(&resendMsg, QIODevice::WriteOnly);

        resendStream.writeBytes(player.name.c_str(), static_cast<uint>(player.name.size()));

        //Wrzuca indexy niepotwierdzonych pozycji (zgubione pakiety) do wiadomości, by móc prosić o retransmisję
        for (unsigned i = 0u; i != player.confirm.size(); ++i)
            if (!player.confirm[i])
                resendStream << i;

        stream.writeBytes(resendMsg, static_cast<uint>(resendMsg.size()));
        resendMsg.clear();
    }
    //Wysyła wiadomość do serwera
    ManagedConnection &connection = client->connection;
    connection.sendDatagram(MessageType::resendPos, msg, -1, connection.address, connection.port);
}
