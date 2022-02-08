#pragma once
#include "player.h"
#include "gamedata.h"
#include "client.h"
#include "server.h"

class GameLogic : public QObject
{
    friend class GameWindow;
    friend class Server;
    friend class Client;
public:
                    GameLogic               (MainWindow* parent, Server* _server, Client *_client, GameData* _data, QLabel* _statusLabel);

    bool            startGame               (qint64 _startTime, qint64 _endTime = 0, unsigned moveIndexData = 0u, int moveIndexCompare = 0, bool bWon = false, std::string winner = "", QByteArray board = "");
    void            drawPlayers             (qreal scale, QPainter &painter, qint64 elapsed);
    //Powoduje wyłonienie zwyciężcy i odliczanie czasu do końca gry
    void            endGame                 (qint64 endTime);
    //Wyłącza grę (nie aplikację)
    void            stopGame                ();
        
    //Deleted
                    GameLogic               (GameLogic&)                = delete;
    GameLogic&      operator=               (const GameLogic)           = delete;

private:
    void            tick                    ();
    void            gameTick                (qint64 elapsed);
    void            checkWin                ();
    
    //Losowanie, czy dany gracz przez określony czas nie będzie rysowany i powstanie dziura w jego linii
    void            rollNoDraw              (Player& player);
    void            checkPlayerCollision    (BoundUnsigned  index);

    //Sieć
    void            readDatagramsClient     ();
    void            readDatagramsServer     ();
    void            sendPosition            ();
    void            askForResendPosition    ();
    QByteArray      buildRepositionMessage  (std::vector<ManagedConnection::ResendData>& resend);
    void            resendPosition          ();

    //Grafika
    void            drawPlayerLine          (Player& player, unsigned playerIndex = 0u, bool bCollision = false);
    void            updateMap               (qint64 elapsed);


    BoundUnsigned   moveIndex,                          //Obecny ruch gracza - indeks tablicy [data] graczy
                    confirmIndex,                       //Tylko klient: indeks tablicy [confirm], która pozwala zauważyć pominięcie pakietu
                    drawIndex;                          //Tylko serwer: indeks odczytu z tablicy [draw] graczy, która pozwala uniknąć samobójstw przy sprawdzaniu kolizji

    //Czasy liczone od epoch
    qint64          startTime               = 0ll,      //Początek gry (synchronizowany z serwerem)
                    endTime                 = 0ll;      //Koniec gry (synchronizowany z serwerem)

    QTimer          tickTimer;                          //Serce logiki: co określony czas wywołuje funkcję tick()

    //Indeksy tablic
    Server          *server;
    Client          *client;

    GameData        *data;

    QLabel          *statusLabel;                       //Status programu w menu głównym

    bool            bServer                 = false;
};
