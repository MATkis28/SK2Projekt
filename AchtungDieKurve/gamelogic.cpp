#include "gamelogic.h"
#include "mainwindow.h"

GameLogic::GameLogic(MainWindow *parent, Server* _server, Client *_client, GameData* _data, QLabel* _statusLabel) :
    QObject(parent), moveIndex(GameSettings::DATA_SEND_TIMEOUT), confirmIndex(GameSettings::DATA_SEND_TIMEOUT), 
    drawIndex(GameSettings::COLLISION_DRAW_FIRST_IGNORE), server(_server), client(_client), data(_data), statusLabel(_statusLabel)
{
    tickTimer.setInterval(static_cast<int>(1000.0 / GameSettings::TICK_RATE));
    tickTimer.callOnTimeout(this, &GameLogic::tick);
    tickTimer.setSingleShot(false);
}

bool GameLogic::startGame(qint64 _startTime, qint64 _endTime, unsigned moveIndexData, int moveIndexCompare, bool bWon, std::string winner, QByteArray board)
{
    if (data->bRunning)
        return false;

    if ((bServer = (server->clients.size() != 0)))
        for (ManagedConnection& con : server->clients)
            con.reset();

    startTime   = _startTime;
    endTime     = _endTime;
    if (data->players.size() < 2)
    {
        statusLabel->setStyleSheet("QLabel { color : rgb(30,30,30); }");
        statusLabel->setText("Waiting for players");

        if (bServer)
            for (ManagedConnection& con : server->clients)
            {
                QByteArray msgSend;
                QDataStream sendStream(&msgSend, QIODevice::WriteOnly);
                sendStream.writeBytes("\x02", 2u);
                con.sendData(MessageType::name, msgSend, server->server.connectionEPOLL);
            }

        return false;
    }
    data->bRunning = true;
    statusLabel->setStyleSheet("QLabel { color : rgb(225,30,20); }");
    statusLabel->setText("Starting");

    data->board = QPixmap(static_cast<int>(GameSettings::BOARD_SIZE), static_cast<int>(GameSettings::BOARD_SIZE));
    data->board.fill(QColor(0, 0, 0));

    if (board.size())
    {
        data->board.loadFromData(board, "BMP");
    }
    else
    {
        QPainter painter(&(data->board));
        painter.setPen(QPen(QColor(0, 255, 255), static_cast<qreal>(static_cast<unsigned long>(GameSettings::WALL_THICKNESS) + 1ul)));
        painter.drawRect(QRect(
            static_cast<int>((GameSettings::WALL_THICKNESS >> 1u) - ((GameSettings::WALL_THICKNESS % 2u) ? 0u : 1u)),
            static_cast<int>((GameSettings::WALL_THICKNESS >> 1u) - ((GameSettings::WALL_THICKNESS % 2u) ? 0u : 1u)),
            static_cast<int>(GameSettings::BOARD_SIZE - (GameSettings::WALL_THICKNESS >> 1u) - (GameSettings::WALL_THICKNESS % 2u)),
            static_cast<int>(GameSettings::BOARD_SIZE - (GameSettings::WALL_THICKNESS >> 1u) - (GameSettings::WALL_THICKNESS % 2u))));
        painter.end();
    }

    data->boardCollision    = data->board.copy();
    data->boardLocal        = data->board.copy();
    data->boardLocalOld     = data->boardLocal.copy();
    for (auto it = data->players.begin(); it != data->players.end(); ++it)
    {
        it->reset(startTime + static_cast<qint64>(GameSettings::FREEZE_TIME));
        QColor color = it->getColor();
        for (auto it2 = data->players.begin(); it2 != it;)
        {
            if (it2->getColor() == color)
            {
                color = it->randomizeColor();
                it->color = color;
                it2 = data->players.begin();
            }
            else ++it2;
        }
    }
                data->bWon      = bWon;
                data->winner    = winner;
                confirmIndex    =
                moveIndex       = moveIndexData;
      moveIndex.compare         =
      confirmIndex.compare      = moveIndexCompare;
                drawIndex       = 0u;
      drawIndex.compare         = 0;
    double      tickTime        = 1000.0 / GameSettings::TICK_RATE;
    unsigned    elapsed         = static_cast<unsigned>((QDateTime::currentMSecsSinceEpoch() - startTime));
    //unsigned    ticksOverhead   = static_cast<unsigned>(elapsed / tickTime);
    unsigned    nextTick        = elapsed % static_cast<unsigned>(tickTime);

    tickTimer.setInterval(static_cast<int>(tickTime));
    QTimer::singleShot(nextTick, this, [this]
        {
            tickTimer.start();
            if (data->bWon)
            {
                statusLabel->setStyleSheet("QLabel { color : rgb(180,180,20); }");
                if (data->winner != "")
                    statusLabel->setText(("The winner is " + data->winner + '!').c_str());
                else
                    statusLabel->setText("A draw!");
            }
            else
            {
                statusLabel->setStyleSheet("QLabel { color : rgb(0,180,20); }");
                statusLabel->setText("Running");
            }
        });

    //Obsłużenie spóźnienia się z wystartowaniem gry na kliencie
    //Teraz algorytm retransmisji robi to lepiej
    /*if (!bServer && elapsed >= GameSettings::FREEZE_TIME)
    {
        ticksOverhead -= GameSettings::FREEZE_TIME / GameSettings::TICK_RATE;
        if (ticksOverhead >= GameSettings::DATA_SEND_TIMEOUT) //jeżeli użytkownik nie namieszał w plikach konfiguracyjnych, to nigdy nie powinno się wydarzyć
        {
            client->close();
            return false;
        }
        //Nie powinno się zdarzyć i może wywołać problemy graficzne, jeżeli jakimś cudem serwer odpali grę później, niż startTime
        //Teoretycznie powinniśmy to obsłużyć, ale jest to bardzo trudne
        //Trzeba by wysłać pakiet z poprawionym czasem albo poprawić błędy w rysowaniu wynikające z tego
        moveIndex = static_cast<unsigned short>(ticksOverhead);

        for (Player& player : data->players)
            for (unsigned i = 1; i <= static_cast<unsigned>(moveIndex); ++moveIndex)
            {
                player.data[i].timestamp = startTime + static_cast<qint64>(GameSettings::FREEZE_TIME);
                player.data[i].noDraw = true;
            }
    }*/

    if (bServer)
        for (Player& player : data->players)
        {
            player.data[0].timestamp = startTime + static_cast<qint64>(GameSettings::FREEZE_TIME);
            player.pos_x = player.data[0].pos_x = QRandomGenerator::global()->bounded(GameSettings::WALL_THICKNESS + (GameSettings::PLAYER_SIZE << 1), GameSettings::BOARD_SIZE - GameSettings::WALL_THICKNESS - (GameSettings::PLAYER_SIZE << 1));
            player.pos_y = player.data[0].pos_y = QRandomGenerator::global()->bounded(GameSettings::WALL_THICKNESS + (GameSettings::PLAYER_SIZE << 1), GameSettings::BOARD_SIZE - GameSettings::WALL_THICKNESS - (GameSettings::PLAYER_SIZE << 1));
            player.angle = player.data[0].angle = QRandomGenerator::global()->bounded(0, static_cast<int>(200*M_PI))/100.0;
        }
    else
    {
        //Będzie pytać o retransmijsę pozycji i uzyskamy w ten sposób pierwszą pozycję gracza
        for (Player& player : data->players)
        {
            if (QDateTime::currentMSecsSinceEpoch() < startTime + static_cast<qint64>(GameSettings::FREEZE_TIME))
                player.data[static_cast<unsigned>(moveIndex)].timestamp = startTime + static_cast<qint64>(GameSettings::FREEZE_TIME);
            else
                player.data[static_cast<unsigned>(moveIndex)].timestamp = QDateTime::currentMSecsSinceEpoch();
            player.confirm[static_cast<unsigned>(moveIndex - 1u)] =
            player.confirm[static_cast<unsigned>(moveIndex)] = false;
            player.data[static_cast<unsigned>(moveIndex-1u)].noDraw = player.data[static_cast<unsigned>(moveIndex)].noDraw = true;
        }
    }
    client->inputSender.setInterval(static_cast<int>(1000.0/static_cast<double>(GameSettings::TICK_RATE)));
    client->inputSender.start();

    return true;
}

void GameLogic::tick()
{
    askForResendPosition();

    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
    if (elapsed >= GameSettings::FREEZE_TIME)
    {
        if (data->bWon && data->bRunning && (QDateTime::currentMSecsSinceEpoch() - endTime) >= GameSettings::WIN_TIME)
        {
            stopGame();
            if (bServer) server->startGame(); //restart
            return;
        }

        if (bServer)
            gameTick(elapsed);
        else
            updateMap(elapsed);
    }
}

void GameLogic::gameTick(qint64 elapsed)
{
    //Oblicz nowy ruch dla każdego gracza
    for (Player& player : data->players)
        if (player.bActive)
        {
            Player::MovementData& newData = player.data[static_cast<unsigned>(moveIndex + 1)];
            newData.timestamp = QDateTime::currentMSecsSinceEpoch();
            newData.input = player.input;
            player.calculateNextMove(moveIndex, (QDateTime::currentMSecsSinceEpoch() - startTime < static_cast<qint64>(GameSettings::FREEZE_TIME) + static_cast<qint64>(GameSettings::GRACE_TIME)));
            //Do wyświetlania na serwerze (serwer też ma okienko!)
            player.pos_x = newData.pos_x;
            player.pos_y = newData.pos_y;
            player.angle = newData.angle;
        }

    if (elapsed >= static_cast<qint64>(GameSettings::FREEZE_TIME) + static_cast<qint64>(GameSettings::GRACE_TIME))
    {
        unsigned index = 0;
        //Kolizja
        for (Player& player : data->players)
        {
            player.checkWallCollision(moveIndex);
            drawPlayerLine(player, index, true);
            ++index;
        }
        //if (!(moveIndex % GameSettings::SMOOTH_INTERVAL))
        checkPlayerCollision(moveIndex);
        //Rysowanie
        for (Player& player : data->players)
        {
            drawPlayerLine(player);
            rollNoDraw(player);
        }
    }

    ++drawIndex;

    sendPosition();
    if (!data->bWon) checkWin();
    ++moveIndex;
}

void GameLogic::checkWin()
{
    if (data->bWon)
        return;

    data->bWon = true;
    data->winner = "";

    for (Player &player : data->players)
        if (player.bActive && !player.bDead)
        {
            if (data->winner == "")
            {
                data->winner = player.name;
                continue;
            }
            data->bWon = false;
            break;
        }

    if (data->bWon)
    {
        statusLabel->setStyleSheet("QLabel { color : rgb(180,180,20); }");
        if (data->winner != "")
            statusLabel->setText(("The winner is " + data->winner + '!').c_str());
        else
            statusLabel->setText("A draw!");

        endTime = QDateTime::currentMSecsSinceEpoch();

        for (ManagedConnection &con : server->clients)
        {
            if (!con.bConnected)
                continue;
            Player *localPlayer = con.getPlayer(data->players);
            if (!localPlayer || !localPlayer->bActive)
                continue;

            QByteArray msg;
            QDataStream stream(&msg, QIODevice::WriteOnly);
            stream << endTime;
            if (data->winner != "")
                stream.writeBytes(data->winner.c_str(), static_cast<uint>(data->winner.size()));
            con.sendData(MessageType::gameEnd, msg, server->server.connectionEPOLL);
        }
    }
}

void GameLogic::stopGame() //Kończy grę 
{
    tickTimer.stop();
    data->bRunning = false;
    if (!bServer)
    {
        client->connection.reset();
        client->inputSender.stop();
    }
    else
    {
        for (unsigned i = 0; i != data->players.size();)
        {
            bool toDelete = true;
            for (ManagedConnection& client : server->clients)
            {
                if (!(data->players[i].name.compare(client.playerName)))
                {
                    toDelete = false;
                    break;
                }
            }
            if (toDelete)
                data->players.erase(data->players.begin() + static_cast<int>(i));
            else ++i;
        }
    }
}

void GameLogic::endGame(qint64 _endTime) //Ogłasza zwyciężcę i odlicza czas do zakończenia gry
{
    this->endTime = _endTime;
    data->bWon = true;

    statusLabel->setStyleSheet("QLabel { color : rgb(180,180,20); }");
    if (data->winner != "")
        statusLabel->setText(("The winner is " + data->winner + '!').c_str());
    else
        statusLabel->setText("A draw!");
}

void GameLogic::checkPlayerCollision(BoundUnsigned index)
{
    QByteArray ba, ba2, ba3;
    QBuffer buff(&ba), buff2(&ba2), buff3(&ba3);
    buff.open(QIODevice::WriteOnly);
    buff2.open(QIODevice::WriteOnly);
    buff3.open(QIODevice::WriteOnly);

    data->boardLocal.save(&buff, "BMP");
    data->boardCollision.save(&buff2, "BMP");
    data->boardLocalOld.save(&buff3, "BMP");
    const unsigned char *bytes  = reinterpret_cast<const unsigned char*>(ba.constData()),
                        *bytes2 = reinterpret_cast<const unsigned char*>(ba2.constData()),
                        *bytes3 = reinterpret_cast<const unsigned char*>(ba3.constData());
    for (unsigned it = 54 + GameSettings::BOARD_SIZE * 3 + 9, end = 54 + 3 * (GameSettings::BOARD_SIZE) * (GameSettings::BOARD_SIZE - 3) + 9; it != end; it += 18)
    {
        for (unsigned end2 = it + GameSettings::BOARD_SIZE * 3 - 18; it != end2; it += 3)
        {
            if (bytes2[it + 2] == 255u && bytes[it + 2] != 0u)
            {
                //qInfo() << bytes2[it] << " " << bytes2[it + 1] << " " << bytes2[it + 2] << " " << bytes2[it + 3];
                Player& killed = data->players[bytes2[it]];
                if (killed.bDead)
                    continue;
                int r, g, b;
                killed.getColor().getRgb(&r, &g, &b);
                if ((bytes3[it] == 0u) && (bytes[it] == b) && (bytes[it + 1] == g) && (bytes[it + 2] == r))
                    continue;
                killed.bDead = true;
                killed.deadPos = QPoint(killed.data[static_cast<unsigned>(index)].pos_x, killed.data[static_cast<unsigned>(index)].pos_y);
            }
        }
    }
    data->boardCollision.fill(QColor(0, 0, 0));
}

void GameLogic::rollNoDraw(Player& player)
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(*QRandomGenerator::global()) < GameSettings::NO_DRAW_CHANCE)
        player.noDrawTicks = QRandomGenerator::global()->bounded(GameSettings::NO_DRAW_MIN, GameSettings::NO_DRAW_MAX);

}

void Client::keyPressed(int key)
{
    Player* player = connection.getPlayer(data->players);
    if (!player)
        return;
    switch (key)
    {
    case Qt::Key_A:
        player->startRotating(true);
        break;
    case Qt::Key_D:
        player->startRotating(false);
        break;
    }
}

void Client::keyReleased(int key)
{
    Player* player = connection.getPlayer(data->players);
    if (!player)
        return;
    switch (key)
    {
    case Qt::Key_A:
        player->stopRotating(true);
        break;
    case Qt::Key_D:
        player->stopRotating(false);
        break;
    }
}
