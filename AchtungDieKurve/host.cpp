#include "host.h"
#include "ui_host.h"
#include "mainwindow.h"

Host::Host(MainWindow *parent, Server* _server, QLabel* _statusLabel, QAction* _actionConnect) : 
    QWidget(parent), ui(new Ui::Host), server(_server), statusLabel(_statusLabel), actionConnect(_actionConnect)
{
    ui->setupUi(this);

    setWindowFlag(Qt::Window);
    //Przycisk host wprowadza server w stan nasłuchiwania
    connect(ui->hostButton, &QPushButton::released, this, [this, parent]
    {
        parent->client.close();
        quint16 port = static_cast<quint16>(ui->portSpinBox->text().toInt());
        if (port < 1 || ui->portSpinBox->text().toInt() > 65535)
        {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Port number must be in range 1-65535"));
            return;
        }

        if (server->listen(htons(port)))
        {
            actionConnect->setDisabled(true);
            statusLabel->setText("Waiting for players");

            static_cast<MainWindow *>(this->parent())->createMocks(QHostAddress("127.0.0.1"), port, 2);
            close();
        }

        updateStatusLabel();
    });
}

void Host::updateStatusLabel()
{
    //if (server->server.isListening())
    if (server->server.bConnected)
    {
        if (ui->statusLabel)
            ui->statusLabel->setText(("Already hosted on port " + std::to_string(htons(server->server.port))).c_str());
        ui->hostButton->setDisabled(true);

    }
    else
    {
        if (ui->statusLabel)
            ui->statusLabel->setText("");
        ui->hostButton->setDisabled(false);
    }
}

Host::~Host()
{
    //QObject::~QObject();
}
