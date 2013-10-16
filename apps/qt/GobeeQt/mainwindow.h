#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <xwlib++/x_objxx.h>

#include "gobeewrap.h"
#include "ui/optionsframe.h"
#include "ui/calldialog.h"
#include "ui/messageform.h"
#include <QSystemTrayIcon>
#include "ui_mainwindow.h"

#include "displayframe.h"
#include "gobeerostermodel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, DisplayFrameClient
{
    Q_OBJECT

    GobeeConnection *_gobee_;
    GobeeRosterModel *gobeeBusModel;
    QSystemTrayIcon *trayIcon;
    QTimer timer;

protected:
    virtual void closeEvent ( QCloseEvent * event );

public:
    explicit MainWindow(QWidget *parent = 0);
    void setConnection(GobeeConnection *conn);
    ~MainWindow();
    void keyPressEvent(QKeyEvent* e);

public Q_SLOTS:
    void showForeignWidget(const char *rjid, const char *,void*, int, int, QWidget **);
    int invalidateForeignWidget(QWidget *, int *err);
    void onTimeOut();
    void showOptionsFrame();
    void showCallDlg(QString jid);
    void showMessageDlg();
    void on_trayIconClicked(QSystemTrayIcon::ActivationReason reason);

private:
    Ui::MainWindow *ui;
    OptionsFrame oframe;
    CallDialog *calldlg;
    MessageForm msgform;

public:
    std::string peer1;
    virtual void frameClientEndCall(std::string rjid, std::string sid);

private slots:
    void on_busView_doubleClicked(const QModelIndex &index);
    void on_busView_customContextMenuRequested(const QPoint &pos);
    void on_busView_activated(const QModelIndex &index);
};

#endif // MAINWINDOW_H
