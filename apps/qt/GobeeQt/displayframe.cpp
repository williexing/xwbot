#include <iostream>

#include "displayframe.h"
#include "ui_displayframe.h"
#include "gobee/gobeehid.h"

#include <QtGui>
#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsView>

extern GobeeHid *getHidDeviceBySessionId(std::string key);

int
DisplayFrame::updateFrame(void)
{
    if (stopped == 3)
        return -1;

    qDebug() << "Updating scene...()" << stopped << " " << w <<"x"<<h<<"\n";
    pixmap.setPixmap(QPixmap::fromImage(QImage((uchar *)tbuf,
                                               w,h,QImage::Format_RGB888)));
    return 0;
}

void
DisplayFrame::setActionClient(DisplayFrameClient *clnt)
{
    m_ActionClient = clnt;
    return;
}

DisplayFrame::DisplayFrame(void *buf, int _w, int _h, char *rjid,
                           char *sid_, QWidget *parent) :
    QFrame(parent),ui(new Ui::DisplayFrame), m_remoteJid(rjid),
    sid(sid_), tbuf((char *)buf), w(_w), h(_h),
    m_ActionClient(0),stopped(0)
{
    ui->setupUi(this);

    pixmap.setPos(0,0);
    scene.addItem(&pixmap);


    ui->graphicsView->grabMouse();
    ui->graphicsView->grabKeyboard();
    ui->graphicsView->setMouseTracking(true);

    ui->graphicsView->setScene(&scene);
    ui->graphicsView->setCacheMode(QGraphicsView::CacheBackground);

    hiddev = getHidDeviceBySessionId(sid);
    hiddev_try_count = 15;

    scene.installEventFilter(this);
}


bool
DisplayFrame::eventFilter(QObject *_qobj, QEvent *_qevent)
{
    if (!hiddev && (--hiddev_try_count > 0))
        hiddev = getHidDeviceBySessionId(sid);

    if(!hiddev)
    {
        return false;
    }

    if (_qevent->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(_qevent);
        hiddev->sendKeyboardEvent(ke);
    }
    else if (
             _qevent->type() == QEvent::GraphicsSceneMouseMove
             || _qevent->type() == QEvent::GraphicsSceneMouseDoubleClick
             || _qevent->type() == QEvent::GraphicsSceneMousePress
             || _qevent->type() == QEvent::GraphicsSceneMouseRelease
             )
    {
        QGraphicsSceneMouseEvent *gme = static_cast<QGraphicsSceneMouseEvent *>(_qevent);
        QPoint qp = gme->scenePos().toPoint();

//        qp.setX(qp.x() + scene.width()/2);
//        qp.setY(qp.y() + scene.height()/2);

        QMouseEvent me(QEvent::MouseMove,qp,
                       gme->button(),gme->buttons(),0);

        ::printf("[GobeeHid] QGraphicsSceneMouseEvent: {%d::%d:%dx%d} EXIT\n",
                 _qevent->type(),(int)gme->buttons(),gme->scenePos().x(),gme->scenePos().y());

        ::printf("[GobeeHid] QMouseEvent: {%d::%d:%dx%d} EXIT\n",
                 me.type(),(int)me.buttons(),me.pos().x(),me.pos().y());

        hiddev->sendMouseEvent(&me);
        ::printf("[GobeeHid] send event ok\n");
        return true;
    }

    return false;
}

DisplayFrame::~DisplayFrame()
{
    delete ui;
}

void
DisplayFrame::on_toolButtonEndCall_clicked()
{
    qDebug() << "Stopping scene... ";
    stopped = 3;
    m_ActionClient->frameClientEndCall(m_remoteJid,sid);
}
