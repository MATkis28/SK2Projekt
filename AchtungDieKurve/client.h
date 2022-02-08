#pragma once
#include "global.h"
#include "gamedata.h"
#include "managedconnection.h"

class Client : public QObject
{
    friend class Connect;
    friend class GameLogic;
public:
                Client          (MainWindow* parent);

    void        setup           (Connect* _connect, GameLogic * _logic, GameData *_data, QLabel *_statusLabel, QAction *_actionHost);
    void        connect         (std::string name, quint32 address, quint16 port);
    void        sendData        (const unsigned char msgType, QByteArray msg = "")               { connection.sendData(msgType, msg); }
    //Zamyka klienta - kończy całą logikę i rozłącza z serwerem
    void        close           ();

    //keyPressed i keyReleased są użyte do poruszania graczem
    void        keyPressed      (int key);
    void        keyReleased     (int key);

    //Deleted
                Client          (Client&)           = delete;
    Client&     operator=       (const Client)      = delete;
    ManagedConnection connection;
private:
    void        ping            ();
    void        readData        ();
    void        timedOut        ();
    void        sendInput       ();
    bool        connected       ();
    void        pool            ();
    QTimer      pinger,         //Wywołuje ping() co [NetworkSettings::PING_INTERVAL]ms by sprawdzić, czy połączenie jest nadal nawiązane
                inputSender,    //Wysyła datagram z pozycją gracza co [1000/GameSettings::TICK_RATE]ms
                pooler,
                timeout;        //Jeżeli przez [NetworkSettings::TIMEOUT]ms nie otrzymamy żadnej informacji od serwera, to uznajemy, że połączenie zostało zerwane
                                //Timeout jest sprawdzany co [NetworkSettings::TIMEOUT]. Może wypadałoby to zmienić na mniejszy czas
    
    Connect     *con;

    GameLogic   *logic;
    GameData    *data;

    QLabel      *statusLabel;   //Status programu w menu głównym
    QAction     *actionHost;    //Przycisk Host w menu Network na wstążce
};
