#pragma once
#include "global.h"
#include "server.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Host; }
QT_END_NAMESPACE

//Tworzy menu, które pozwala założyć serwer
class Host : public QWidget
{
public:
            Host                (MainWindow* parent, Server* _server, QLabel* _statusLabel, QAction* _actionConnect);
           ~Host                ()              override;

    //Aktualizuje status serwera (QLabel znajduje się pod polem do wpisania adresu IP)
    void    updateStatusLabel   ();
    
    //Deleted
            Host                (Host&)         = delete;
    Host&   operator=           (const Host)    = delete;
private:
    std::unique_ptr<Ui::Host> ui;

    Server  *server;

    QLabel  *statusLabel;           //Status programu w menu głównym
    QAction *actionConnect;         //Przycisk Connect w menu Network na wstążce
};
