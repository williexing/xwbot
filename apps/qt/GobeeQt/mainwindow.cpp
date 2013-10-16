#include "mainwindow.h"
//#include "gobeebusmodel.h"
#include "ui_mainwindow.h"

#include "gobeewrap.h"
#include "gobee/gobeehid.h"
#include <string>

#include <map>
#include <iostream>

#include <QApplication>
#include <QMenuBar>
#include <QToolBar>


void
MainWindow::closeEvent ( QCloseEvent * event )
{
    //    setVisible(false);
    setConnection(NULL);
    event->accept();
}

void
MainWindow::setConnection(GobeeConnection *conn)
{
    GobeeConnection *oldconn = _gobee_;

    _gobee_ = conn;

    if (gobeeBusModel)
        gobeeBusModel->setRootNode(NULL);

    if (oldconn)
        oldconn->stop();

    if (_gobee_)
    {
        gobeeBusModel->setRootNode(_gobee_->getBus());
        _gobee_->start();
        QObject::connect(_gobee_,
                         SIGNAL(postWidgetShow(const char *,const char *,void *,int,int,QWidget **)),
                         this,
                         SLOT(showForeignWidget(const char *,const char *,void *,int,int,QWidget **)));

        QObject::connect(_gobee_,SIGNAL(postWidgetInvalidate(QWidget*,int*)),this,
                         SLOT(invalidateForeignWidget(QWidget *,int *)));

        QObject::connect(&timer,SIGNAL(timeout()),this,SLOT(onTimeOut()));
        oframe.setConnection(_gobee_);
    }

}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    trayIcon(new QSystemTrayIcon(this)),
    _gobee_(NULL)
{
    ui->setupUi(this);
    ui->menuBar->setVisible(false);

//    oframe = new OptionsFrame();
//    msgform =new MessageForm();

    gobeeBusModel = new GobeeRosterModel(this);
    ui->busView->setModel(gobeeBusModel);
    ui->busView->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(ui->actionExit,
                     SIGNAL(triggered()),qApp,
                     SLOT(closeAllWindows()));

    QObject::connect(ui->actionOptions,
                     SIGNAL(triggered()),this,
                     SLOT(showOptionsFrame()));

    //    QObject::connect(ui->actionCall,
    //                     SIGNAL(triggered()),this,
    //                     SLOT(showCallDlg(QString)));

    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     this, SLOT(on_trayIconClicked(QSystemTrayIcon::ActivationReason)));

    trayIcon->setToolTip(QString("Gobee Qt"));
    trayIcon->setIcon(QIcon(":/icons/OK.png"));
    trayIcon->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void
MainWindow::keyPressEvent(QKeyEvent *e)
{
    int key = e->key();
    if (key == Qt::Key_Escape)
    {
        ui->menuBar->setVisible(!ui->menuBar->isVisible());
        e->ignore();
    }
    else
        if (key == Qt::Key_Home /*
                        && e->modifiers() & Qt::ControlModifier*/)
        {
            std::cout << "'M' pressed\n";
            if(ui->busView->viewMode() == QListView::IconMode)
                ui->busView->setViewMode(QListView::ListMode);
            else
                ui->busView->setViewMode(QListView::IconMode);
            e->ignore();
        }
        else
            e->accept();
}

void MainWindow::showForeignWidget(const char *rjid, const char *sid, void *buf, int _w, int _h,QWidget **qwgt)
{
    DisplayFrame *dframe = new DisplayFrame(buf,_w,_h ,(char *)rjid,(char *)sid);
    if (qwgt)
    {
        *qwgt = (QWidget *)dframe;
    }
    dframe->setActionClient(this);
    dframe->setUpdatesEnabled(true);
    dframe->show();
}

void MainWindow::showOptionsFrame()
{
    std::cout << "[MainWindow] Calling to showOptionsFrame()\n";
    oframe.show();
}

void MainWindow::showCallDlg(QString jid = "")
{
    bool video = false;
    bool media = false;

    CallDialog calldlg(this);
    calldlg.setRemoteJid(jid);
    calldlg.setWindowTitle(QString("Call to ") + jid);
    calldlg.show();

    if(calldlg.exec() == calldlg.Accepted)
    {
        std::string peer = calldlg.getRemoteJid().toStdString();
        std::cout << "[MainWindow] Calling to " << peer;
        switch(calldlg.getCallType())
        {
        case 0:
            break;
        case 1:
            video = true;
            break;
        case 2:
            video = true;
            media = true;
            break;
        default:
            return;
        }
        _gobee_->callTo(peer,video,media);
    }
}

void MainWindow::showMessageDlg()
{
    msgform.show();
}

void MainWindow::on_trayIconClicked(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        this->setVisible(!this->isVisible());
        break;
    case QSystemTrayIcon::MiddleClick:
        //        showMessage();
        break;
    default:
        ;
    }
}

int MainWindow::invalidateForeignWidget(QWidget *wgt,int *err)
{
    *err = ((DisplayFrame *)wgt)->updateFrame();
    return *err;
}

void MainWindow::onTimeOut()
{
    std::cout << "[MainWindow] Calling to " << peer1;
    _gobee_->callTo(peer1,true,false);
}

void
MainWindow::on_busView_doubleClicked(const QModelIndex &index)
{
//    QString jid  = gobeeBusModel->data(index,Qt::UserRole).toString();
//    showCallDlg(jid);/*
//    calldlg->setRemoteJid(jid);

//    if(calldlg->exec() == calldlg->Accepted)
//    {
//        std::string peer = calldlg->getRemoteJid().toStdString();
//        std::cout << "[MainWindow] Calling to " << peer;
//        _gobee_->callTo(peer,true,true);
//    }*/
}

void MainWindow::on_busView_customContextMenuRequested(const QPoint &pos)
{
    std::cout << "[MainWindow] Menu request peer1\n";

    // for most widgets
    QPoint globalPos = ui->busView->mapToGlobal(pos);
    // for QAbstractScrollArea and derived classes you would use:
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);


    QModelIndex idx = ui->busView->indexAt(pos);
    QMenu myMenu;
    if(idx.isValid())
    {
        myMenu.addAction("Voice Call");
        myMenu.addAction("Video Call");
        myMenu.addAction("Media Call");
        myMenu.addAction("Message");
        //    myMenu.addAction("Send file");
        myMenu.addSeparator();
        myMenu.addAction("Info");
        // ...
    }
    else
    {
        myMenu.addAction("Switch view");
    }

    QAction* selectedItem = myMenu.exec(globalPos);
    if (selectedItem)
    {
        if (selectedItem->text() == "Voice Call")
        {
            QModelIndexList list = ui->busView->selectionModel()->selectedIndexes();
            QModelIndex index = list.at(0);
            QString jid  = gobeeBusModel->data(index,Qt::UserRole).toString();
            _gobee_->callTo(jid.toStdString(),false,false);
        }
        else
            if (selectedItem->text() == "Video Call")
            {
                QModelIndexList list = ui->busView->selectionModel()->selectedIndexes();
                QModelIndex index = list.at(0);
                QString jid  = gobeeBusModel->data(index,Qt::UserRole).toString();
                _gobee_->callTo(jid.toStdString(),true,false);
            }
            else
                if (selectedItem->text() == "Media Call")
                {
                    QModelIndexList list = ui->busView->selectionModel()->selectedIndexes();
                    QModelIndex index = list.at(0);
                    QString jid  = gobeeBusModel->data(index,Qt::UserRole).toString();
                    _gobee_->callTo(jid.toStdString(),true,true);
                }
                else
                    if (selectedItem->text() == "Message")
                    {
                        showMessageDlg();
                    }
                    else if (selectedItem->text() == "Switch view")
                    {
                        if(ui->busView->viewMode() == QListView::IconMode)
                            ui->busView->setViewMode(QListView::ListMode);
                        else
                            ui->busView->setViewMode(QListView::IconMode);
                    }
    }
    else
    {
        // nothing was chosen
    }
}

void
MainWindow::on_busView_activated(const QModelIndex &index)
{
    QString jid  = gobeeBusModel->data(index,Qt::UserRole).toString();
    showCallDlg(jid);
}



/**
 * DisplayFrameClient interface functions
 */
void
MainWindow::frameClientEndCall(std::string rjid, std::string sid)
{
    _gobee_->endCall(rjid, sid);
}

