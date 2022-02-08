#include "connect.h"
#include "ui_connect.h"
#include "mainwindow.h"

Connect::Connect(MainWindow *parent, Client* _client, QLabel* _statusLabel, QAction* _actionHost) : 
    QWidget(parent), ui(new Ui::Connect), client(_client), statusLabel(_statusLabel), actionHost(_actionHost)
{
    ui->setupUi(this);

    setWindowFlag(Qt::Window);
    timeout.setSingleShot(true);
    timeout.setInterval(static_cast<int>(NetworkSettings::TIMEOUT));

    connect(&timeout, &QTimer::timeout, this, [this]
    {
        ui->connectButton->setDisabled(false);
        this->client->connection.close();
        client->pinger.stop();
        client->pooler.stop();
        QMessageBox::critical(this, tr("Error"),
                              tr("Connection timeout"));
    });

    //Wymusza wpisanie poprawnego adresu IPv4
    QString ipNumber = "(([0-1]?[0-9]?[0-9])|(2[0-4][0-9])|(25[0-5]))\\.";
    ui->addressEdit->setValidator( new QRegExpValidator( QRegExp('^' + ipNumber + ipNumber + ipNumber + "(([0-1]?[0-9]?[0-9])|(2[0-4][0-9])|(25[0-5]))$", Qt::CaseInsensitive), this));
    connect(ui->connectButton, &QPushButton::released, this, [this]
    {
        if (!ui->nameEdit->text().size())
        {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Not a valid name"));
            return;
        }
        timeout.stop();
        client->close();
        ui->connectButton->setDisabled(true);

        timeout.start();
        QString address = ui->addressEdit->text();
        quint16 port = static_cast<quint16>(ui->portSpinBox->text().toInt());
        client->connect(ui->nameEdit->text().toStdString(), htonl(QHostAddress(address).toIPv4Address()), htons(port));
    });
}

void Connect::errorHandle(QAbstractSocket::SocketError )
{
    timeout.stop();
    /*
    if (this->isVisible())
        QMessageBox::critical(this, tr("Error"),
                          tr("The port %1 is not available")
                          .arg(client->connection.errorString().c_str()));
    */
    client->connection.close();
    ui->connectButton->setDisabled(false);
}

void Connect::connected()
{
    actionHost->setDisabled(true);
    this->close();
}

void Connect::closeEvent(QCloseEvent *)
{
    timeout.stop();
    if (!client->connection.bConnected)
        client->connection.close();
    ui->connectButton->setDisabled(false);
}

Connect::~Connect()
{
    //QObject::~QObject();
}
