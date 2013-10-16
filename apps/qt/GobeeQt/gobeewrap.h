#ifndef GOBEEWRAP_H
#define GOBEEWRAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <dlfcn.h>

#include <QMutex>
#include <QSize>
#include <QThread>
#include <QWaitCondition>
#include <QtSql>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[GobeeQt] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <plugins/sessions_api/sessions.h>

class GobeeConnection : public QThread
{
    Q_OBJECT
    gobee::__x_object *bus;
    QString m_lastTime;
    QSqlDatabase sdb;

private:
    std::string _pwd;

public:
    // user settings
    std::string _jid;
    std::string _host;
    std::string _domain;
    std::string _stunserver;

    std::string getJid ()
    {
        if (bus)
        {
            const x_string_t s =
                    (const x_string_t) bus->__getattr((const char*)"jid");
            if(s)
                _jid = s;
        }
        return _jid;
    }
    std::string getHost () {
        if (bus)
        {
            const x_string_t s =
                    (const x_string_t) bus->__getattr((const char*)"hostname");
            if(s)
                _host = s;
        }
        return _host;
    }
    std::string getDomain()
    {
        if (bus)
        {
            const x_string_t s =
                    (const x_string_t) bus->__getattr((const char*)"domain");
            if(s)
                _domain = s;
        }
        return _domain;
    }
    std::string getStunServer()
    {
        if (bus)
        {
            const x_string_t s =
                    (const x_string_t) bus->__getattr((const char*)"stunserver");
            if(s)
                _stunserver = s;
        }
        return _stunserver;
    }

    void setJid (std::string j);
    void setPw (std::string h);
    void setHost (std::string h);
    void setDomain (std::string d);
    void setStunServer (std::string s);

protected:
    void run();
    void load_plugins(const char *_dir);
    void readDb(std::string jd,
                std::string pw);

public:
    virtual ~GobeeConnection();
    GobeeConnection(const std::string jid, const std::string pw);
    int __pushWidgetToShow(const char *rjid, const char *sid,
                            void *buf, int w, int h, QWidget **qwgt);
    int __pushWidgetInvalidate(QWidget *wgt);
    gobee::__x_object *getBus() const;

    bool callTo(std::string peer, bool video, bool media);
    void endCall(std::string peerjid, std::string sid);

    void stop();

Q_SIGNALS:
    int postWidgetShow(const char *rjid, const char *sid,
                        void *buf, int w, int h, QWidget **qwgt);
    int postWidgetInvalidate(QWidget *wgt, int *err);
};

int pushWidgetToShow(gobee::__x_object *gbus,
                      const char *rjid, const char *sid, void *buf,
                     int w, int h, void **qwgt);

#endif // GOBEEWRAP_H
