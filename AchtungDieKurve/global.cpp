#include "global.h"
#include "player.h"

unsigned        GameSettings::BOARD_SIZE                    = 512;
unsigned        GameSettings::WALL_THICKNESS                = 2;
unsigned        GameSettings::PLAYER_SIZE                   = 10;
unsigned        GameSettings::LINE_SIZE                     = 3;
unsigned        GameSettings::NO_DRAW_MIN                   = 18;
unsigned        GameSettings::NO_DRAW_MAX                   = 35;
unsigned        GameSettings::COLLISION_DRAW_FIRST_IGNORE   = 20;
unsigned        GameSettings::DATA_SEND_TIMEOUT             = 300;
unsigned        GameSettings::WIN_TIME                      = 6000;
unsigned        GameSettings::GRACE_TIME                    = 4000;
unsigned        GameSettings::FREEZE_TIME                   = 3999;
unsigned        GameSettings::TICK_RATE                     = 60;
unsigned        GameSettings::FREEZE_FONT_SIZE              = 64;
unsigned        GameSettings::WIN_FONT_SIZE                 = 64;
unsigned        GameSettings::NAME_FONT_SIZE                = 12;
float           GameSettings::SPEED                         = 0.06f;
float           GameSettings::ANGLE_SPEED                   = 0.00225f;
double          GameSettings::NO_DRAW_CHANCE                = 0.014;

unsigned        NetworkSettings::TIMEOUT                    = 5000;
unsigned        NetworkSettings::PING_INTERVAL              = 200;
unsigned short  NetworkSettings::UDP_PORT_START             = 14000; //Od tego portu serwer zaczyna przyznawać poszczególnym klientom porty UDP
unsigned        NetworkSettings::POOL_TIME                  = 25;
