#include <QThread>
#include <QTimer>
#include <QTranslator>
#include <QtGui/QApplication>

#include <QtSql>

#include "mainwindow.h"

#undef DEBUG_PRFX
#define DEBUG_PRFX "[GobeeQt] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include "gobeewrap.h"
#include "gobeeapplication.h"

int main(int argc, char *argv[])
{
    GobeeApplication a(argc, argv);
    MainWindow w;

    std::string jd("");
    std::string pw("");

    if (argc >= 3)
    {
        if( a.arguments().contains("-username") )
        {
            int i =  a.arguments().indexOf("-username");
            jd =  a.arguments().at(i+1).toStdString();
        }

        if( a.arguments().contains("-pwd") )
        {
            int i =  a.arguments().indexOf("-pwd");
            pw =  a.arguments().at(i+1).toStdString();
        }
    }

    //    QResource::registerResource("resources.qrc");

    /* create main window */
    w.show();

    // new connection
    GobeeConnection gbeeconn(jd,pw);
    w.setConnection(&gbeeconn);

    return a.exec();
}
