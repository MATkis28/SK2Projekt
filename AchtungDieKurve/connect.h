#pragma once
#include "global.h"
#include "client.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Connect; }
QT_END_NAMESPACE

//Tworzy menu, które pozwala połączyć się z serwerem
class Connect : public QWidget
{
    friend class Client;
public:
                Connect         (MainWindow* parent, Client* _client, QLabel* _statusLabel, QAction* _actionHost);
                ~Connect        ()                          override;
    //Deleted
                Connect         (Connect&)                  = delete;
    Connect&    operator=       (const Connect)             = delete;
protected:
    void        closeEvent      (QCloseEvent *event)        override;

private:
    void        errorHandle     (QAbstractSocket::SocketError socketError);
    void        connected       ();

    std::unique_ptr<Ui::Connect> ui;

    Client      *client;

    QLabel      *statusLabel;           //Status programu w menu głównym 
    QAction     *actionHost;            //Przycisk Host w menu Network na wstążce

    QTimer      timeout;
};
