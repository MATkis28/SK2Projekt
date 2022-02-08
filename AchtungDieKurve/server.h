#pragma once
#include "global.h"
#include "gamedata.h"
#include "managedconnection.h"

class Server : public QObject
{
    friend class GameLogic;
    friend class Host;
public:
    Server                          (MainWindow         *parent);

    void setup                      (Host               *_host,         GameLogic           * _logic,
                                     GameData           *_data,         QLabel              *_statusLabel,
                                     QAction            *_actionConnect);
    bool listen                     (quint16             port);
    void acceptConnection           ();
    void terminateConnection        (int con);
    void checkTimeout               ();
    void close                      ();

    void startGame                  ();
    void sendStartGameMessage       (ManagedConnection  &client);
    //Deleted
    Server(Server&)                 = delete;
    Server& operator=(const Server) = delete;
private:
    void pool                       ();
    void readData                   (ManagedConnection *connection);
    //QTcpServer                    server;
    ManagedConnection               server;
    //Połączenie UDP do przesyłania pozycji do graczy

    QTimer                          timeout,
                                    pooler;

    Host                            *host;
    GameLogic                       *logic;
    GameData                        *data;
    QLabel                          *statusLabel;           //Status programu w menu głównym   
    QAction                         *actionConnect;         //Przycisk Connect w menu Network na wstążce

    std::vector<ManagedConnection>  clients;
};
