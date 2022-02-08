#pragma once
#include "global.h"
#include "gamelogic.h"
#include "gamedata.h"

class GameWindow : public QWidget
{
public:
                GameWindow      (QWidget        *parent);

    //Niestety nie możemy zawrzeć inicjalizacji tych elementów w konstruktorze GameWindow (a przynajmniej nie wiemy jak),
    //ponieważ ta klasa jest częścią "mainwindow.ui"
    void        setGame         (GameLogic      *_logic, GameData *_data)  
    { this->logic = _logic; this->data = _data; }
    //Deleted
                GameWindow      (GameWindow&)                   = delete;
    GameWindow& operator=       (const GameWindow)              = delete;
protected:
    void        paintEvent      (QPaintEvent    *event)         override;
    void        resizeEvent     (QResizeEvent   *event)         override;

private:
    GameLogic   *logic          = nullptr;
    GameData    *data           = nullptr;
    qreal       scale           = 1.0;                      //Skala okienka z grą
};
