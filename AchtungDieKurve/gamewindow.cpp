#include "gamewindow.h"
#include "mainwindow.h"
#include <QtMath>

GameWindow::GameWindow(QWidget *parent) : QWidget(parent)
{
    QTimer::singleShot(0, [this] { scale = static_cast<float>(this->rect().width())/static_cast<float>(GameSettings::BOARD_SIZE); });
}

void GameWindow::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (!logic || !data->bRunning)
    {
        //todo: sprawdzić, czy to czasem nie jest CPU-heavy i wrzucić do Timera, jeżeli jest
        this->update(); //bez tego przestaniemy dostawać aktualizacje widgetu, gdy będzie już potrzebny. W ten sposób sztucznie utrzymujemy widget przy życiu
        return;
    }

    QFont font("Arial", 32);

    QPainter painter(this);
    QPixmap &board = data->boardLocal;

    painter.setPen(QColor(255,0,0));
    painter.drawPixmap(QRect(0, 0, this->size().width(), this->size().height()), board.scaled(this->size()));

    font.setPixelSize(static_cast<int>(GameSettings::FREEZE_FONT_SIZE));
    painter.setFont(font);

    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - logic->startTime;

    if (elapsed < GameSettings::FREEZE_TIME) 
        painter.drawText(this->rect(), Qt::AlignCenter,
                         tr(std::to_string(qCeil((static_cast<double>(GameSettings::FREEZE_TIME) - static_cast<double>(elapsed))/1000.0)).c_str()));
    if (data->bWon)
    {
        qint64 elapsedEnd = QDateTime::currentMSecsSinceEpoch() - logic->endTime;

        font.setPixelSize(static_cast<int>(GameSettings::WIN_FONT_SIZE));
        painter.setFont(font);

        painter.setPen(QColor(255,255,0));

        painter.drawText(this->rect(), Qt::AlignCenter, tr(std::to_string(qCeil((static_cast<double>(GameSettings::WIN_TIME) - static_cast<double>(elapsedEnd))/1000.0)).c_str()));
    }
    logic->drawPlayers(scale, painter, elapsed);
    this->update();
}

void GameWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    int width   = event->size().width();
    int height  = event->size().height();
    int minSize = static_cast<int>(fmin(width, height));

    if (width != height)
    {
        this->resize(minSize, minSize);
        this->move(10 + (((width > height) ? (width - height) : 0) >> 1), 30 + (((height > width) ? (height - width) : 0) >> 1));
        return;
    }
    scale = static_cast<float>(width)/static_cast<float>(GameSettings::BOARD_SIZE);
    this->update();
}
