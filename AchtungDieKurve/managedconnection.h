#pragma once
#include "global.h"
#include "player.h"

class Player;
class ManagedConnection
{
public:
    //Wskazuje na dane, które serwer musi wysłać ponownie w wyniku niedotarcia pakietku UDP
    struct ResendData
    {
        ResendData() {}
        ResendData(Player* player) : player(player)     {}

        ResendData(const ResendData &resend)            { *this = resend; }

        ResendData& operator=(const ResendData &rhs)
        {
            this->player                = rhs.player;
            this->confirm               = rhs.confirm;
            return *this;
        }

        Player                  *player = nullptr;
        std::vector<unsigned>   confirm;
    };
    ManagedConnection() = default;

    ManagedConnection(int socket, quint32 _address, quint16 _port) : tcpSocket(socket), address(_address), port(_port)
    {
        lastReceived.start();
        bConnected = true;
    }
    bool                        connect         (quint32 address, quint16 port);
    bool                        connected       ();
    bool                        bindTcpSocket   (quint16 _port);
    bool                        bindUdpSocket   (quint16 _port);
    bool                        readData        (unsigned char &msgType, QByteArray &msg);
    bool                        readDatagram    (unsigned char &msgType, QByteArray &msg, quint32 *sender = nullptr, quint16 *senderPort = nullptr);
    void                        sendData        (const unsigned char msgType, QByteArray msg = "", int EPOLL = -1);
    void                        sendDatagram    (const unsigned char msgType, QByteArray msg = "", int EPOLL = -1, quint32 destAddr = 0, quint16 destPort = 0);
    void                        sendQueuedDatagram(int EPOLL = -1);
    void                        sendQueuedData  (int EPOLL = -1);
    bool                        listen          (quint16 _port);
    void                        pool            ();
    void                        reset           ();
    void                        close           ();
    Player*                     getPlayer       (std::vector<Player> &players);
    void                        error           (std::string title, bool bCritical = true);

    int                         tcpSocket       = -1,
                                udpSocket       = -1,
                                connectionEPOLL = -1;
    QByteArray                  messages;

    std::vector<std::pair<sockaddr_in, QByteArray>> datagrams;

    quint32                     address         = 0u,
                                token           = 0u;
    quint16                     port            = 0u,
                                udpPort         = 0u;
    bool                        bConnected      = false;

    QByteArray                  data;
    QElapsedTimer               lastReceived;
    std::string                 playerName      = "";
};
