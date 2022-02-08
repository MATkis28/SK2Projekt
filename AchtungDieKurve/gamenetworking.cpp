#include "gamelogic.h"

void GameLogic::sendPosition(BoundUnsigned moveIndex)
{
    return;
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);
    stream << static_cast<unsigned>(moveIndex);
    for (Player &player : data->players)
    {
        QByteArray playerData;
        QDataStream streamPlayer(&playerData, QIODevice::WriteOnly);
        if (!player.bActive)
            continue;

        streamPlayer.writeBytes(player.name.c_str(), player.name.size());
        streamPlayer    << static_cast<unsigned>(moveIndex)
                        << player.data[static_cast<unsigned>(moveIndex)];
        stream.writeBytes(playerData, playerData.size());
    }

    for (ManagedConnection &con : server->clients)
        con.sendDatagram(MessageType::sendPos, msg);
}

QByteArray GameLogic::buildRepositionMessage(std::vector<ManagedConnection::ResendData> &resend)
{
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);
    stream << static_cast<unsigned>(moveIndex);
    for (ManagedConnection::ResendData &res : resend)
    {
        QByteArray  playerMsg;
        QDataStream playerStream(&playerMsg, QIODevice::WriteOnly);
        Player      *player = res.player;

        if (!player->bActive)
            continue;

        playerStream.writeBytes(player->name.c_str(), player->name.size());

        for (unsigned i = 0u; i != res.confirm.size(); ++i)
            playerStream << i << res.player->data[i];

        stream.writeBytes(playerMsg, static_cast<unsigned>(playerMsg.size()));
    }

    return msg;
}

void GameLogic::resendPosition()
{
    QByteArray msg;
    QDataStream stream(&msg, QIODevice::WriteOnly);

    if (!bServer)
    {
        for (Player &player : data->players)
        {
            if (!player.bActive)
                continue;

            QByteArray resendMsg;
            QDataStream resendStream(&resendMsg, QIODevice::WriteOnly);
            resendStream.writeBytes(player.name.c_str(), player.name.size());
            for (unsigned i = 0u; i != player.confirm.size(); ++i)
                if (!player.confirm[i])
                    resendStream << i;
            stream.writeBytes(resendMsg, resendMsg.size());
        }
        client->connection.sendDatagram(MessageType::askResendPos, msg, client->connection.port);
    }
}

void GameLogic::readDatagrams()
{
    if (bServer)
    {
        QByteArray msg;
        unsigned char msgType;
        quint32 sender;
        unsigned short senderPort;
        while (server->positionConnection.readDatagram(msgType, msg, &sender, &senderPort))
        {
            QDataStream stream(&msg, QIODevice::ReadOnly);
            if (msgType == MessageType::askResendPos)
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
                QByteArray msg = buildRepositionMessage(resend);
                //if (QRandomGenerator::global()->bounded(0, 100) < 5)
                server->positionConnection.sendDatagram(MessageType::sendPos, msg, sender, senderPort);
            }
        }
    }
    else
    {
        QByteArray msg;
        unsigned char msgType;
        while (client->connection.readDatagram(msgType, msg))
        {
            QDataStream stream(&msg, QIODevice::ReadOnly);
            if (msgType == MessageType::sendPos)
            {
                unsigned tick;
                stream >> tick;
                for (BoundUnsigned i = moveIndex; static_cast<unsigned>(i) != static_cast<unsigned>(tick); ++i)
                {
                    for (Player &player : data->players)
                        player.confirm[static_cast<unsigned>(i)] = false;
                }
                moveIndex = tick;
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
                    char *playerName;
                    unitStream.readBytes(playerName, size);
                    while (!unitStream.atEnd())
                    {
                        unsigned index;
                        unitStream >> index;
                        Player::MovementData mov;
                        unitStream >> mov;

                        Player *player = data->findPlayer(playerName);
                        if (player)
                        {
                            player->data[index] = mov;
                            player->confirm[index] = true;
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
                }
            }
        }
    }
}
