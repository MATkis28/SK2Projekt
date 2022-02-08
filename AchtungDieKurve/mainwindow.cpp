#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow),
        server((ui->setupUi(this), this)),
        client(this),
        host(this,      &server, ui->statusLabel, ui->actionConnect),
        con(this,       &client, ui->statusLabel, ui->actionHost),
        logic(this,     &server, &client, &data, ui->statusLabel),
        data(this)
{
    this->installEventFilter(this);
    ui->gameWindow->setGame(&logic, &data);
    server.setup(&host, &logic, &data, ui->statusLabel, ui->actionConnect),
    client.setup(&con, &logic, &data, ui->statusLabel, ui->actionHost),
    connect(ui->actionHost,         &QAction::triggered, this, [this] { con.close(); host.show(); });
    connect(ui->actionDisconnect,   &QAction::triggered, this, [this] { server.close(); client.close(); host.updateStatusLabel(); });
    connect(ui->actionConnect,      &QAction::triggered, this, [this] { host.close(); con.show(); });
    con.close();
    host.close();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    this->ui->gameWindow->resize(this->size() - QSize(20, 60));
    this->ui->statusLabel->resize(this->size().width() - 20, 30);
    QWidget::resizeEvent(event);
}

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        client.keyPressed(static_cast<QKeyEvent*>(event)->key());
        return true;
    }
    else if (event->type() == QEvent::KeyRelease)
    {
        client.keyReleased(static_cast<QKeyEvent*>(event)->key());
        return true;
    }
    return false;
}

MainWindow::~MainWindow()
{
    //QMainWindow::~QMainWindow();
    server.close();
    client.close();
}

//Pomocniczna funkcja tworząca więcej okienek, przy pomocy których można testować program
//Jest automatyczne łączenie (po to są parametry address i port)
void MainWindow::createMocks(QHostAddress address, quint16 port, unsigned number)
{
    if (this->parent())
        return;
    for (unsigned i = 0; i != number; ++i)
    {
        mockClients.emplace_back(new MainWindow(this));
        mockClients.back()->setAttribute(Qt::WA_DeleteOnClose);
        mockClients.back()->show();
        mockClients.back()->client.connect(std::to_string(i), htonl(address.toIPv4Address()), htons(port));
    }
}
