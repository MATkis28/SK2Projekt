#pragma once
#include "player.h"

Player::Player(std::string name)
{
    color = randomizeColor();
    this->name = name;
}

void Player::reset(qint64 timestamp)
{
    data.clear();
    data.reserve(GameSettings::DATA_SEND_TIMEOUT);
    for (unsigned i = 0; i != GameSettings::DATA_SEND_TIMEOUT; ++i)
        data.emplace_back();

    confirm.clear();
    confirm.reserve(GameSettings::DATA_SEND_TIMEOUT);
    for (unsigned i = 0; i != GameSettings::DATA_SEND_TIMEOUT; ++i)
        confirm.push_back(true);

    draw.clear();
    draw.reserve(GameSettings::COLLISION_DRAW_FIRST_IGNORE);
    for (unsigned i = 0; i != GameSettings::COLLISION_DRAW_FIRST_IGNORE; ++i)
        draw.emplace_back();

    //pos_x = data[0].pos_x         = QRandomGenerator::global()->bounded(GameSettings::WALL_THICKNESS + (GameSettings::PLAYER_SIZE<<1), GameSettings::BOARD_SIZE - GameSettings::WALL_THICKNESS - (GameSettings::PLAYER_SIZE<<1));
    //pos_y = data[0].pos_y         = QRandomGenerator::global()->bounded(GameSettings::WALL_THICKNESS + (GameSettings::PLAYER_SIZE<<1), GameSettings::BOARD_SIZE - GameSettings::WALL_THICKNESS - (GameSettings::PLAYER_SIZE<<1));
    //angle = data[0].angle         = QRandomGenerator::global()->bounded(0, 2*M_PI);
    pos_x                           = -2.0 * GameSettings::PLAYER_SIZE;
    pos_y                           = -2.0 * GameSettings::PLAYER_SIZE;
    angle                           = -2.0 * GameSettings::PLAYER_SIZE;
    data[0].timestamp               = timestamp;

    bDead                           = false;
    bActive                         = true;
}

void Player::calculateNextMove(BoundUnsigned index, bool noDraw)
{
    if (bDead)
    {
        MovementData &prevData      = data[static_cast<unsigned>(index)];
        MovementData &processedData = data[static_cast<unsigned>(++index)];
        processedData.demise        = true;
        processedData.angle         = prevData.angle;
        processedData.pos_x         = deadPos.x();
        processedData.pos_y         = deadPos.y();
        return;
    }
    if (this->noDrawTicks != 0)
        --this->noDrawTicks;

    MovementData &prevData          = data[static_cast<unsigned>(index)];
    MovementData &processedData     = data[static_cast<unsigned>(++index)];

    if (!(processedData.noDraw      = noDraw))  //Przekazanie [true] w [noDraw] powoduje brak rysowania linii gracza
          processedData.noDraw      = static_cast<bool>(this->noDrawTicks);

    qreal timeFactor                = static_cast<qreal>(processedData.timestamp - prevData.timestamp);

    processedData.angle = prevData.angle;
    if (processedData.input & (1u << 1u))
        processedData.angle -= static_cast<qreal>(GameSettings::ANGLE_SPEED) * timeFactor;

    if (processedData.input & (1u << 2u))
        processedData.angle += static_cast<qreal>(GameSettings::ANGLE_SPEED) * timeFactor;

    processedData.pos_x = prevData.pos_x + timeFactor * static_cast<qreal>(GameSettings::SPEED) * qCos(prevData.angle);
    processedData.pos_y = prevData.pos_y + timeFactor * static_cast<qreal>(GameSettings::SPEED) * qSin(prevData.angle);
}

void Player::checkWallCollision(BoundUnsigned index)
{
    if (bDead || !bActive)
        return;
    MovementData &processedData = this->data[static_cast<unsigned>(index)];

    if (processedData.pos_x < /*PLAYER_SIZE*/0)
        bDead = true;
    else if (processedData.pos_x > GameSettings::BOARD_SIZE /*- PLAYER_SIZE*/)
        bDead = true;

    if (processedData.pos_y < /*PLAYER_SIZE*/0)
        bDead = true;
    if (processedData.pos_y > GameSettings::BOARD_SIZE /*- PLAYER_SIZE*/)
        bDead = true;

    if (bDead)
        deadPos = QPoint(processedData.pos_x, processedData.pos_y);
}

