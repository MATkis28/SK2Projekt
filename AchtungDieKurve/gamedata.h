#pragma once
#include <global.h>
#include "player.h"

struct GameData : public QObject
{
                        GameData(QObject *parent) : QObject(parent) {}

    //Istnieje konwersja z const char* na QByteArray, więc można wygodnie szukać gracza po nazwie
    Player*             findPlayer(QByteArray name)
    {
        std::string playerName = name.toStdString();

        auto it = std::find(players.begin(), players.end(), playerName);
            
        if (it == players.end()) return nullptr;

        return &*it;
    }

    //Deleted
    GameData(GameData&)                     = delete;
    GameData& operator=(const GameData)     = delete;

    QPixmap             boardLocal,                         //Obraz, na którym klient przewiduje następny stan gry. Serwer używa tej mapy do rysowania prawidłowego obrazu
                        board,                              //Potwierdzony obraz z serwera. Serwer nie używa tej mapy
                        boardCollision,                     //Obraz serwera, na którym rysuje się czerwone linie, które potem się sprawdza jako potencjalne miejsca kolizji graczy
                        boardLocalOld;                      //Kopia obrazu serwera sprzed GameSettings::COLLISION_DRAW_FIRST_IGNORE czasu, która pozwala uniknąć samobójstw przy sprawdzaniu kolizji

    bool                bWon                = false,        //Czy znamy zwyciężcę i czekamy na nową rundę
                        bRunning            = false;        //Mówi, czy gra trwa i czy jest sens np. rysować okienko
    std::string         winner              = "";

    std::vector<Player> players;
};
