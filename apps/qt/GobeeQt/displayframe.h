#ifndef DISPLAYFRAME_H
#define DISPLAYFRAME_H

#include <QFrame>
#include <QtGui/QGraphicsView>
#include <QtGui>
#include <QPaintEvent>
#include <QPaintEngine>

#include <stdio.h>
#include <string>

#include "gobeegraphicsscene.h"
#include "gobee/gobeehid.h"

namespace Ui {
    class DisplayFrame;
}

class DisplayFrameClient
{
public:
    virtual void frameClientEndCall(std::string rjid, std::string sid) {}
    virtual void frameClientMuteCall(std::string sid) {}
    virtual void frameClientResumeCall(std::string sid) {}
    virtual void frameClientPauseCall(std::string sid) {}
};

class DisplayFrame : public QFrame
{
    Q_OBJECT
public:
    explicit DisplayFrame(void *buf, int _w, int _h, char *rjid, char *sid, QWidget *parent = 0);
    ~DisplayFrame();

    void setActionClient(DisplayFrameClient *clnt);
    int updateFrame(void);
    bool eventFilter(QObject *, QEvent *);

private slots:
    void on_toolButtonEndCall_clicked();

private:
    Ui::DisplayFrame *ui;
    GobeeGraphicsScene scene;
    QGraphicsPixmapItem pixmap;
    char *tbuf;
    int w;
    int h;
    std::string m_remoteJid;
    std::string sid;
    GobeeHid *hiddev;
    DisplayFrameClient *m_ActionClient;

public:
    int stopped;

private:
    int hiddev_try_count;
};

#endif // DISPLAYFRAME_H
