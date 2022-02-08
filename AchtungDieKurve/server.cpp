#include "server.h"
#include "mainwindow.h"

Server::Server(MainWindow *parent) :
    QObject(parent)//, server(new QTcpServer(this)), positionConnection(new QTcpSocket())
{
    timeout.setSingleShot(false);
    timeout.setInterval(static_cast<int>(NetworkSettings::TIMEOUT));
    timeout.callOnTimeout(this, &Server::checkTimeout);

    pooler.callOnTimeout(this,    &Server::pool);
    pooler.setInterval(NetworkSettings::POOL_TIME);
}

void Server::setup(Host *_host, GameLogic *_logic, GameData *_data, QLabel *_statusLabel, QAction* _actionConnect)
{
    host            = _host;
    logic           = _logic;
    data            = _data;
    statusLabel     = _statusLabel;
    actionConnect   = _actionConnect;
    timeout.start();

    //connect(&server, &QTcpServer::newConnection, this, &Server::acceptConnection);
}

void Server::close()
{
    pooler.stop();
    server.close();
    for (ManagedConnection &client : clients)
        client.close();
    clients.clear();
    statusLabel->setStyleSheet("QLabel { color : rgb(30,30,30); }");
    statusLabel->setText("Not connected");
    actionConnect->setDisabled(false);

    logic->stopGame();
    data->players.clear();
}

void Server::checkTimeout()
{
    for (unsigned i = 0; i != clients.size(); ++i)
    {
        ManagedConnection &connection = clients.at(i);
        if (connection.lastReceived.elapsed() > static_cast<int>(NetworkSettings::TIMEOUT))
        {
            terminateConnection(connection.tcpSocket);
            --i;
            //QMessageBox::critical(static_cast<QWidget *>(this->parent()), tr("Server"),
            //                      tr("Timeout!"));
        }
    }
}

bool Server::listen(quint16 port)
{
    if (!server.listen(port))
        return false;
    pooler.start();
    //UDP socket
    return true;
}
