#include "gamelogic.h"

void GameLogic::updateMap(qint64 elapsed)
{
    if (confirmIndex.compare < moveIndex.compare && confirmIndex.data < moveIndex.data)
        client->timedOut();
    static BoundUnsigned    lastComputed;
    static qint64           lastTimestamp;
    QPainter painterBoard(&(data->board));

    //Rysuje obraz, który został potwierdzony danymi z serwera
    bool bFine = true;
    while (static_cast<unsigned>(confirmIndex) != static_cast<unsigned>(moveIndex + 1u))
    {
        //Sprawdza, czy wszyscy gracze w obecnym ,,ticku'' mają potwierdzone dane
        for (Player &player : data->players)
        {
            if (!player.bActive || player.bDead) continue;
            Player::MovementData    &processedData  = player.data[static_cast<unsigned>(confirmIndex)];
            if (!player.confirm[static_cast<unsigned>(confirmIndex)])
                bFine = false;
            else if (processedData.demise)
            {
                if (!player.bDead)
                {
                    player.bDead = true;
                    player.deadPos = QPointF(processedData.pos_x, processedData.pos_y);
                }
            }
        }
        if (!bFine)
            break;
        
        //RysowanieV
        for (Player &player : data->players)
        {
            if (!player.bActive || player.bDead)
                continue;
            painterBoard.setPen(QPen(player.getColor(), GameSettings::LINE_SIZE));
            Player::MovementData    &prevData       = player.data[static_cast<unsigned>(confirmIndex - 1)],
                                    &processedData  = player.data[static_cast<unsigned>(confirmIndex)];

            if (processedData.demise)
            {
                if (!player.bDead)
                {
                    player.bDead = true;
                    player.deadPos = QPointF(processedData.pos_x, processedData.pos_y);
                }
                continue;
            }

            if (processedData.noDraw)
                continue;

            QPointF beg = QPointF(prevData.pos_x,         prevData.pos_y),
                    end = QPointF(processedData.pos_x,    processedData.pos_y);

            painterBoard.drawLine(beg, end);
        }

        ++confirmIndex;
    }
    //Kopia potwierdzonego obrazu do obecnie rysowanego
    data->boardLocal = data->board.copy(data->boardLocal.rect());

    //Rysowanie przewidywanego stanu gry
    QPainter painterLocalBoard(&(data->boardLocal));
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    if (static_cast<unsigned>(confirmIndex) != static_cast<unsigned>(lastComputed))
        lastTimestamp = currentTime;

    qreal step = 0.0;
    for (BoundUnsigned conf = confirmIndex, nextConfirmedIndex = confirmIndex; static_cast<unsigned>(conf) != static_cast<unsigned>(moveIndex + 1u); ++conf)
    {
        for (Player &player : data->players)
        {
            if (!player.bActive || player.bDead)
                continue;

            painterLocalBoard.setPen(QPen(player.getColor(), GameSettings::LINE_SIZE));
            Player::MovementData    &prevData       = player.data[static_cast<unsigned>(conf - 1u)];
            Player::MovementData    &processedData  = player.data[static_cast<unsigned>(conf)];

            //Jeżeli natkniemy się na potwierdzone dane, to używamy ich, zamiast naszych przewidywań
            //Sprawdzamy, czy takie dane nie wystapują zawczasu i podczas przetwarzania niepotwierdzonych stosujemy
            //interpolację liniową do danych potwierdzonych, o ile takie istnieją
            if (conf >= nextConfirmedIndex)
            {
                BoundUnsigned i = conf + 1u;
                for (; static_cast<unsigned>(i) != static_cast<unsigned>(moveIndex + 1u); ++i)
                    if (player.confirm[static_cast<unsigned>(i)])
                        break;
                nextConfirmedIndex = i;

                qint64 time = player.data[static_cast<unsigned>(nextConfirmedIndex)].timestamp;
                if (static_cast<unsigned>(nextConfirmedIndex) == static_cast<unsigned>(moveIndex + 1u))
                    time = lastTimestamp;

                step = static_cast<qreal>(time - prevData.timestamp)/static_cast<qreal>(static_cast<unsigned>(nextConfirmedIndex - static_cast<unsigned>(conf)) + 1u);
            }

            if (!player.confirm[static_cast<unsigned>(conf)])
            {
                //Można zrobić osobny index do zapamiętywania, które
                processedData.timestamp = static_cast<qint64>(prevData.timestamp) + static_cast<qint64>(step);
                processedData.input = prevData.input;
                player.calculateNextMove(conf - 1u, elapsed < static_cast<qint64>(GameSettings::FREEZE_TIME) + static_cast<qint64>(GameSettings::GRACE_TIME));
            }

            if (prevData.noDraw)
                continue;

            QPointF beg = QPointF(prevData.pos_x, prevData.pos_y),
                    end = QPointF(processedData.pos_x, processedData.pos_y);

            painterLocalBoard.drawLine(beg, end);
        }
    }

    if (static_cast<unsigned>(confirmIndex) != static_cast<unsigned>(lastComputed))
    {
        //Alternatywny sposób rysowania
        //for (Player &player : data->players)
           //player.data[static_cast<unsigned>(moveIndex + 1)] = player.data[static_cast<unsigned>(moveIndex)];
        lastComputed = confirmIndex;
    }
    //Ostatni ruch jest zawsze przewidywany (ponieważ inaczej i tak musielibyśmy dostać informację od serwera o nowym ticku gry...
    //a przecież po to jest przewidywanie, by zachować płynność gry, gdy nie mamy komunikacji z serwerem)
    for (Player &player : data->players)
    {
        if (!player.bActive || player.bDead)
                continue;

        painterLocalBoard.setPen(QPen(player.getColor(), GameSettings::LINE_SIZE));

        Player::MovementData    &prevData       = player.data[static_cast<unsigned>(moveIndex)];
        Player::MovementData    &processedData  = player.data[static_cast<unsigned>(moveIndex + 1)];
        //Player::MovementData    copy            = prevData;
        processedData.timestamp                 = currentTime;

        processedData.input = prevData.input;
        player.calculateNextMove(moveIndex, elapsed < static_cast<qint64>(GameSettings::FREEZE_TIME) + static_cast<qint64>(GameSettings::GRACE_TIME));

        QPointF beg = QPointF(prevData.pos_x, prevData.pos_y),
                end = QPointF(processedData.pos_x, processedData.pos_y);

        if (!prevData.noDraw)
            painterLocalBoard.drawLine(beg, end);

        //Dane do wyświetlania "głów" graczy
        player.pos_x = processedData.pos_x;
        player.pos_y = processedData.pos_y;
        player.angle = processedData.angle;
    }
}

void GameLogic::drawPlayers(qreal scale, QPainter &painter, qint64 elapsed)
{
    for (Player &player : data->players)
    {
        if (!player.bActive)
            continue;

        painter.setPen(player.getColor());
        painter.setBrush(player.getColor());

        if (player.bDead)
            painter.drawEllipse(QRectF(scale * (player.deadPos.x() - GameSettings::PLAYER_SIZE/2.0), scale * (player.deadPos.y() - GameSettings::PLAYER_SIZE/2.0), scale * GameSettings::PLAYER_SIZE, scale * GameSettings::PLAYER_SIZE));
        else if (player.pos_x > -static_cast<double>(GameSettings::PLAYER_SIZE))  //oznacza, że klient jeszcze nie pobrał informacji o pozycji z serwera
        {
            //Rysuje gracza
            painter.drawEllipse(QRectF(scale * (player.pos_x - GameSettings::PLAYER_SIZE/2.0), scale * (player.pos_y - GameSettings::PLAYER_SIZE/2.0), scale * GameSettings::PLAYER_SIZE, scale * GameSettings::PLAYER_SIZE));
            //Rysuje strzałkę, która wskazuje kierunek gracza
            if (elapsed < GameSettings::FREEZE_TIME)
            {
                QPointF directionPoint = scale * 
                    QPointF(player.pos_x + static_cast<qreal>((static_cast<unsigned long>(GameSettings::PLAYER_SIZE) << 1u)) * qCos(player.angle), player.pos_y + static_cast<qreal>((static_cast<unsigned long>(GameSettings::PLAYER_SIZE) << 1u) * qSin(player.angle)));
                painter.drawLine(scale * QPointF(player.pos_x, player.pos_y),
                                 directionPoint);
                painter.drawLine(directionPoint,
                                 scale * QPointF(player.pos_x + static_cast<qreal>(GameSettings::PLAYER_SIZE) * 1.5 * qCos(player.angle-(M_PI_4/2)), player.pos_y + static_cast<qreal>(GameSettings::PLAYER_SIZE) * 1.5 * qSin(player.angle-(M_PI_4/2))));
                painter.drawLine(directionPoint,
                                 scale * QPointF(player.pos_x + static_cast<qreal>(GameSettings::PLAYER_SIZE) * 1.5 * qCos(player.angle+(M_PI_4/2)), player.pos_y + static_cast<qreal>(GameSettings::PLAYER_SIZE) * 1.5 * qSin(player.angle+(M_PI_4/2))));

                QFont               font("Arial", static_cast<int>(GameSettings::NAME_FONT_SIZE));
                QFontMetrics        fm(font);
                int                 pixelsWide = fm.horizontalAdvance(player.name.c_str());
                int                 pixelsHigh = fm.height();
                painter.setFont(font);

                painter.drawText(scale * QPointF(player.pos_x - (pixelsWide >> 1), qRound(player.pos_y) + pixelsHigh),
                                 tr(player.name.c_str()));
            }
        }
    }
}

void GameLogic::drawPlayerLine(Player &player, unsigned playerIndex, bool bCollision)
{
    //if (moveIndex % GameSettings::SMOOTH_INTERVAL)
    //    return;
    if (!player.bActive || player.bDead)
        return;

    if (bCollision)
    {
        BoundUnsigned           index           = moveIndex;
        Player::MovementData    &prevData       = player.data[static_cast<unsigned>(index)],
                                &processedData  = player.data[static_cast<unsigned>(++index)];

        if (processedData.noDraw)
            return;

        //Rysowanie na obrazie lini potencjalnej kolizji
        QPainter painterBoardCollision(&(data->boardCollision));

        QPointF beg = QPointF(prevData.pos_x, prevData.pos_y),
                end = QPointF(processedData.pos_x, processedData.pos_y);

        painterBoardCollision.setPen(QPen(QColor(255, playerIndex % 256, static_cast<int>(playerIndex)), GameSettings::LINE_SIZE));
        painterBoardCollision.drawLine(beg, end);
        return;
    }

    if (bServer)
    {
        //Do rysowania starego obrazu gry (unikanie samobójstw w kolizji)
        Player::DrawData &draw = player.draw[static_cast<unsigned>(drawIndex)];
        QPainter painterBoardOld(&(data->boardLocalOld));
        painterBoardOld.setPen(QPen(player.getColor(), GameSettings::LINE_SIZE));
        painterBoardOld.drawLine(draw.beg, draw.end);
    }

    BoundUnsigned           index           = moveIndex;
    Player::MovementData    &prevData       = player.data[static_cast<unsigned>(index)],
                            &processedData  = player.data[static_cast<unsigned>(++index)];

    if (processedData.noDraw)
        return;

    QPointF beg = QPointF(prevData.pos_x, prevData.pos_y),
            end = QPointF(processedData.pos_x, processedData.pos_y);

    //Zapisujemy obecną linię na końcu kolejki rysowania starego obrazu
    if (bServer)
    {
        BoundUnsigned drawTick                              = drawIndex;
        player.draw[static_cast<unsigned>(--drawTick)].beg  = beg;
        player.draw[static_cast<unsigned>(drawTick)].end    = end;
    }
    //Rysowanie na wyświetlanym obrazie
    QPainter painterBoardLocal(&(data->boardLocal));
    painterBoardLocal.setPen(QPen(player.getColor(), GameSettings::LINE_SIZE));
    painterBoardLocal.drawLine(beg, end);
}
