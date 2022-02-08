#pragma once
#include "global.h"
#include "host.h"
#include "server.h"
#include "client.h"
#include "connect.h"
#include "gamelogic.h"
#include "gamedata.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    friend class Host;
public:
                MainWindow  (QWidget *parent = nullptr);
                ~MainWindow ()                                      override;

    //Debug
    void        createMocks (QHostAddress address, quint16 port, unsigned number);
    
    //Deleted
    MainWindow(MainWindow&) = delete;
    MainWindow& operator=   (const MainWindow) = delete;
protected:
    bool        eventFilter (QObject *watched, QEvent *event)       override;
    void        resizeEvent (QResizeEvent *event)                   override;

private:
    void        gameTick    ();
    std::shared_ptr<Ui::MainWindow>             ui;
    std::vector<MainWindow*>                    mockClients;        //Debug

    Server      server;
    Client      client;

    Host        host;
    Connect     con;

public:
    GameLogic   logic;
    GameData    data;
};
