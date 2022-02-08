#pragma once
#include "global.h"

struct BoundUnsigned;
class Player
{
    friend class GameLogic;
    friend class Client;
public:
    Player(std::string name);
    Player(const Player &player)
    {
        *this = player;
    }

    //Struktura przesyłana w sieci
    struct MovementData
    {
        qint64          timestamp   = 0;

        qreal           pos_x       = 0.0,
                        pos_y       = 0.0,
                        angle       = 0.0;

        //1 - brak
        //2 - lewo
        //4 - prawo
        //6 - brak (teraz łatwo zdjąć najmłodszy 2. albo 3. bit i mieć wiadomość 'lewo' lub 'prawo')
        unsigned char   input       = 1u;
        bool            noDraw      = true,
                        demise      = false;



        friend QDataStream& operator<<(QDataStream& stream, const MovementData& data)
        {
            stream  << data.pos_x
                    << data.pos_y
                    << data.angle

                    << data.input
                    << data.timestamp

                    << data.noDraw
                    << data.demise;

            return stream;
        }

        friend QDataStream& operator>>(QDataStream& stream, MovementData& data)
        {
            stream  >> data.pos_x
                    >> data.pos_y
                    >> data.angle

                    >> data.input
                    >> data.timestamp

                    >> data.noDraw
                    >> data.demise;

            return stream;
        }
    };

    //Struktura użyta do rozstrzygania kolizji przez serwer. Pozwala uniknąć samobójstw
    struct DrawData
    {
        QPointF         beg         = QPointF(0.0, 0.0),
                        end         = QPointF(0.0, 0.0);
    };
    
    //Przywraca gracza do stanu fabrycznego i ustawia
    void reset                  (qint64 time);
    //Oblicza następną pozycję na bazie poprzedniej i obecnej oraz upłyniętego czasu (różnicy timestamp)
    void calculateNextMove      (BoundUnsigned index, bool noDraw = false);
    void checkWallCollision     (BoundUnsigned index);

    QColor randomizeColor       ()
    {
        return color = QColor(QRandomGenerator::global()->bounded(186,255),
                              QRandomGenerator::global()->bounded(186,255),
                              QRandomGenerator::global()->bounded(186,255));
    }

    QColor getColor             ()
    {
        if (bDead)
            return QColor(color.red() - 70, color.green() - 70, color.blue() - 70);
        return color;
    }

    void startRotating          (bool left) { input |=   left ? (1<<1) : (1<<2); }
    void stopRotating           (bool left) { input &= ~(left ? (1<<1) : (1<<2)); }

    Player& operator=(const Player &rhs)
    {
        this->bActive               = rhs.bActive;
        this->name                  = rhs.name;
        this->color                 = rhs.color;
        this->token                 = rhs.token;
        this->input                 = rhs.input;
        this->data                  = rhs.data;
        this->confirm               = rhs.confirm;
        this->draw                  = rhs.draw;
        this->noDrawTicks           = rhs.noDrawTicks;
        this->bDead                 = rhs.bDead;
        this->deadPos               = rhs.deadPos;

        this->pos_x                 = rhs.pos_x;
        this->pos_y                 = rhs.pos_y;
        this->angle                 = rhs.angle;
        return *this;
    }

    bool operator==(std::string rhs) const { return name == rhs; }

    std::string                 name                = "";
    QColor                      color               = QColor(0,0,0);
    quint32                     token               = 0u;           //Token połączenia, by umożliwić reconnect i zabronić innym kradnięcie gracza
                                                                    //po rozłączaniu oraz jeszcze gorszą sytuację - rozłączać oryginalnego
    bool                        bActive             = false;        //Czy gracz jest w grze, czy dopiero dołączył w trakcie gry
    bool                        bDead               = false;
private:
    unsigned char               input               = 1u;
    qreal                       pos_x               = -2.0,
                                pos_y               = -2.0 * GameSettings::PLAYER_SIZE,
                                angle               = -2.0 * GameSettings::PLAYER_SIZE;
    //Wektor, ponieważ nie znamy na jakich ustawieniach gry działa serwer oraz możemy podczas jednego uruchomienia aplikacji łączyć się do kilku
    std::vector<MovementData>   data;
    //Dba o synchronizację danych z serwerem
    std::vector<bool>           confirm;

    //Serwer
    std::vector<DrawData>       draw;
    //Serwer
    unsigned                    noDrawTicks         = 0u;           //Czas, podczas którego nie rysujemy nic - wtedy powstaje dziura w linii gracza
    
    QPointF                     deadPos;
};
