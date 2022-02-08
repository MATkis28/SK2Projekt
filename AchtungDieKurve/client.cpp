#include "client.h"
#include "mainwindow.h"

Client::Client(MainWindow *parent)
: QObject(parent)//, connection(new QTcpSocket)
{
    pinger      .setInterval(static_cast<int>(NetworkSettings::PING_INTERVAL));
    pinger      .callOnTimeout(this, &Client::ping);

    timeout     .setInterval(static_cast<int>(NetworkSettings::TIMEOUT));
    timeout     .setSingleShot(false);
    timeout     .callOnTimeout(this, &Client::timedOut);

    inputSender .setSingleShot(false);
    inputSender .callOnTimeout(this, &Client::sendInput);

    pooler      .setSingleShot(false);
    pooler      .callOnTimeout(this, &Client::pool);
    pooler      .setInterval(1);
}

void Client::setup(Connect* _connect, GameLogic * _logic, GameData *_data, QLabel *_statusLabel, QAction *_actionHost)
{
    con = _connect;
    logic = _logic;
    data = _data;
    statusLabel = _statusLabel;
    actionHost = _actionHost;

    #ifdef QtSocket
    QTcpSocket *&socket = connection.socket;
    QTcpSocket::connect(socket, &QTcpSocket::readyRead,     this,       &Client::readData);
    QTcpSocket::connect(socket, &QTcpSocket::connected,     this,       &Client::connected);
    QTcpSocket::connect(socket, &QTcpSocket::errorOccurred, con,        &Connect::errorHandle);
    #endif
}

void Client::connect(std::string name, quint32 address, quint16 port)
{
    connection.playerName = name;
    if (!connection.connect(address, port))
        timeout.stop();
    else
        pooler.start();
}

bool Client::connected()
{
    if (!connection.connected())
        return false;
    con->close();
    return true;
}

void Client::ping()
{
    if (connection.bConnected)
        connection.sendData(MessageType::ping);
}

void Client::timedOut()
{
    close();

    QMessageBox::critical(static_cast<QWidget *>(this->parent()), tr("Error"),
                          tr("Connection timeout"));
}

void Client::close()
{
    pooler.stop();
    logic->stopGame();
    timeout.stop();
    pinger.stop();

    actionHost->setDisabled(false);
    statusLabel->setStyleSheet("QLabel { color : rgb(30,30,30); }");
    statusLabel->setText("Not connected");

    connection.close();
}
