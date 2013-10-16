#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>
#include <dlfcn.h>

#include <iostream>

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

#include "displayframe.h"
#include "gobee/gobeehid.h"
#include "gobee/gobeemsg.h"

#include "gobeewrap.h"

std::map<std::string, GobeeHid *> sid2hid;
std::map<std::string, GobeeMsg *> sid2msgdev;
std::map<gobee::__x_object *, GobeeConnection *> bus2conn;

int
pushWidgetToShow(gobee::__x_object *gbus, const char *rjid, const char *__sid,
                 void *buf, int w, int h, void **qwgt)
{
    GobeeConnection *gbee = bus2conn[gbus];
    gbee->__pushWidgetToShow(rjid, __sid, buf, w, h, (QWidget **)qwgt);
}

int pushWidgetInvalidate(gobee::__x_object *gbus, void *qwgt)
{
    GobeeConnection *gbee;
    if (!qwgt)
    {
        ::printf("pushWidgetInvalidate) GobeeConnection NULL\n");
        return -1;
    }

    gbee = bus2conn[gbus];
    return gbee->__pushWidgetInvalidate((QWidget *)qwgt);
}

int
registerHidDeviceWithSessionId(const char *sessId, GobeeHid *dev)
{
    std::string key = std::string(sessId);
    sid2hid[key] = dev;
}

GobeeHid *
getHidDeviceBySessionId(std::string sessId)
{
    return sid2hid[sessId];
}

int
registerMessageIODeviceWithSessionId(const char *sessId, GobeeMsg *dev)
{
    std::string key = std::string(sessId);
    sid2msgdev[key] = dev;
}

GobeeMsg *
getMessageIODeviceBySessionId(std::string sessId)
{
    return sid2msgdev[sessId];
}

void
GobeeConnection::readDb (std::string jd,
                         std::string pw)
{
    std::string stunsrv;
    x_obj_attr_t attrs =
    { 0, 0, 0 };

    sdb = QSqlDatabase::addDatabase("QSQLITE");
    sdb.setDatabaseName("gobee.db");

    if (sdb.open())
    {
        QSqlQuery query = QSqlQuery(sdb);
        bool r = query.exec("SELECT name FROM sqlite_master "
                            "WHERE type='table' AND name='tlv';");

        if(!r || !query.next())
        {
            ::printf("%d\n",__LINE__);
            query.exec("create table tlv (_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "_key KEY TEXT, value TEXT);");
        }
    }

    QSqlQuery query = QSqlQuery(sdb);

    if(jd.length() > 0 && pw.length() > 0)
    {
        bool b = query.exec("select value from tlv where _key='username';");
        if(!b
                || !query.next())
        {
            if (!query.exec(QString() + "insert into tlv('_key','value') "
                            "values('username','" + jd.c_str() + "');"))
            {
                qDebug() << sdb.lastError().text();
            }

            if(!query.exec(QString() + "insert into tlv('_key','value') "
                           "values('password','" + pw.c_str() + "');"))
            {
                qDebug() << sdb.lastError().text();
            }
            sdb.commit();
        }
        else
        {
            ::printf("Do not need to update DB\n");
        }
    }
    else
    {
        if(query.exec("select value from tlv where _key='username';"))
        {
            if(query.next())
            {
                ::printf("Using username='%s'\n",
                         query.value(0).toString().toStdString().c_str());
                jd = query.value(0).toString().toStdString();
            }
        }
        else
        {
            BUG();
            exit (-1);
        }

        if(query.exec("select value from tlv where _key='password';"))
        {
            if(query.next())
            {
                pw = query.value(0).toString().toStdString();
            }
        }
        else
        {
            BUG();
            exit (-1);
        }
    }

    setattr(const_cast<char *>("pword"), const_cast<char *>(pw.c_str()), &attrs);
    setattr(const_cast<char *>("jid"),const_cast<char *>(jd.c_str()), &attrs);


    // read connection settings
    if(query.exec("select value from tlv where _key='stunserver';"))
    {
        if(query.next())
        {
            ::printf("Using stun host='%s'\n",
                     query.value(0).toString().toStdString().c_str());
            stunsrv = query.value(0).toString().toStdString();
            setattr(const_cast<char *>("stunserver"),
                    const_cast<char *>(stunsrv.c_str()), &attrs);
        }
    }

    *bus += &attrs;
}

void
GobeeConnection::setJid (std::string j)
{
    QSqlQuery query = QSqlQuery(sdb);
    if(query.exec("select value from tlv where _key='username';"))
    {
        if(query.next())
        {
            if (!query.exec(QString() + "update tlv set value='" + j.c_str()
                            + "' where _key='username';"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
        else
        {
            if (!query.exec(QString() + "insert into tlv('_key','value') "
                            "values('username','" + j.c_str() + "');"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
    }
}

void
GobeeConnection::setPw (std::string p)
{
    QSqlQuery query = QSqlQuery(sdb);
    if(query.exec("select value from tlv where _key='password';"))
    {
        if(query.next())
        {
            if (!query.exec(QString() + "update tlv set value='" + p.c_str()
                            + "' where _key='password';"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
        else
        {
            if (!query.exec(QString() + "insert into tlv('_key','value') "
                            "values('password','" + p.c_str() + "');"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
    }
}

void
GobeeConnection::setHost (std::string h)
{
    QSqlQuery query = QSqlQuery(sdb);
    if(query.exec("select value from tlv where _key='hostname';"))
    {
        if(query.next())
        {
            if (!query.exec(QString() + "update tlv set value='" + h.c_str()
                            + "' where _key='hostname';"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
        else
        {
            if (!query.exec(QString() + "insert into tlv('_key','value') "
                            "values('hostname','" + h.c_str() + "');"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
    }
}

void
GobeeConnection::setDomain (std::string d)
{
    _domain = d;
}

void
GobeeConnection::setStunServer (std::string s)
{
    _stunserver = s;
    if (bus)
    {
        bus->__setattr(X_STRING("stunserver"),X_STRING(s.c_str()));
    }

    QSqlQuery query = QSqlQuery(sdb);
    if(query.exec("select value from tlv where _key='stunserver';"))
    {
        if(query.next())
        {
            if (!query.exec(QString() + "update tlv set value='" + _stunserver.c_str()
                            + "' where _key='stunserver';"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
        else
        {
            if (!query.exec(QString() + "insert into tlv('_key','value') "
                            "values('stunserver','" + _stunserver.c_str() + "');"))
            {
                qDebug() << sdb.lastError().text();
            }
        }
    }
}

void
GobeeConnection::run()
{
    forever
    {
        bus->commit();
    }
}

void
GobeeConnection::stop()
{
    bus->on_remove();
    this->terminate();
}

void
GobeeConnection::load_plugins(const char *_dir)
{
    char libname[64];
    DIR *dd;
    struct dirent *de;
    char *dir = (char *) _dir;

    if ((dd = opendir(dir)) == NULL)
    {
        return;
    }

    while ((de = readdir(dd)))
    {
        snprintf(libname, sizeof(libname), "%s/%s", dir, de->d_name);
        if (!dlopen(libname, RTLD_LAZY | RTLD_GLOBAL))
        {
        }
        else
            printf("OK! loaded %s\n", libname);
    }
}

GobeeConnection::~GobeeConnection()
{
    if (!isRunning());

    sdb.close();

    return;
}

GobeeConnection::GobeeConnection(const std::string jid,
                                 const std::string pw) : QThread()
{
    load_plugins("./");
    load_plugins("./output");

    /* instantiate '#xbus' object from 'gobee' namespace */
    bus = new ("gobee","#xbus") gobee::__x_object();
    BUG_ON(!bus);

    readDb(jid,pw);

    /* register ourself */
    bus2conn[bus] = this;
}

int GobeeConnection::__pushWidgetToShow(
        const char *rjid, const char *sid, void *buf, int w, int h, QWidget **qwgt)
{
    Q_EMIT postWidgetShow(rjid, sid, buf,w,h, qwgt);
}

int GobeeConnection::__pushWidgetInvalidate(QWidget *wgt)
{
    int err = 0;
    Q_EMIT postWidgetInvalidate(wgt,&err);
    ::printf("pushWidgetInvalidate) __pushWidgetInvalidate() = %d\n",err);
    return err;
}

gobee::__x_object *
GobeeConnection::getBus() const
{
    return bus;
}

bool
GobeeConnection::callTo(std::string peer, bool video = false, bool media = false)
{
    x_object *_chan_;
    x_object *_sess_ = NULL;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };

    const char *jid = peer.c_str();

    std::cout << "[GobeeConnection] Calling to " << peer;
    TRACE("Calling to %s\n",jid);

    BUG_ON(!jid);

    _sess_ = X_OBJECT(x_session_open(
                          jid, X_OBJECT(bus), &hints, X_CREAT));

    if (!_sess_)
    {
        BUG();
    }

    std::cout << "[GobeeConnection] Calling to " << peer;
    TRACE("Calling to %s\n",jid);

#if 1
    /* open/create audio channel */
    _chan_ = X_OBJECT(x_session_channel_open2(
                          X_OBJECT(_sess_), "m.A"));

    setattr(_XS("mtype"), _XS("audio"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);

    attr_list_clear(&hints);
    x_session_channel_set_transport_profile_ns(
                X_OBJECT(_chan_), _XS("__icectl"),
            #if 0
                _XS("http://www.google.com/transport/p2p"), &hints);
#else
                _XS("urn:xmpp:jingle:transports:ice-udp:1"), &hints);
#endif
    x_session_channel_set_media_profile_ns(X_OBJECT(_chan_),
                                           _XS("__rtppldctl"),_XS("jingle"));

    /* add channel payloads */
    setattr("clockrate", "16000", &hints);
    x_session_add_payload_to_channel(_chan_, _XS("110"), _XS("speex"), MEDIA_IO_MODE_OUT, &hints);
    /* clean attribute list */
    attr_list_clear(&hints);
#endif

    if(video)
    {
        /* open/create video channel */
        _chan_ = x_session_channel_open2(
                    X_OBJECT(_sess_), "m.V");

        setattr(_XS("mtype"), _XS("video"), &hints);
        _ASGN(X_OBJECT(_chan_), &hints);

        attr_list_clear(&hints);
        x_session_channel_set_transport_profile_ns(
                    X_OBJECT(_chan_), _XS("__icectl"),
            #if 0
                    _XS("http://www.google.com/transport/p2p"), &hints);
#else
                    _XS("urn:xmpp:jingle:transports:ice-udp:1"), &hints);
#endif

        x_session_channel_set_media_profile_ns(
                    X_OBJECT(_chan_),
                    _XS("__rtppldctl"),_XS("jingle"));

        /* add channel payloads */
        attr_list_clear(&hints);
        setattr(_XS("clockrate"), _XS("90000"), &hints);
        setattr(_XS("width"), _XS("320"), &hints);
        setattr(_XS("height"), _XS("240"), &hints);
        setattr(_XS("sampling"), _XS("YCbCr-4:2:0"), &hints);

#if 0
        x_session_add_payload_to_channel(
                    X_OBJECT(_chan_),
                    _XS("96"), _XS("THEORA"),
                    MEDIA_IO_MODE_OUT, &hints);
#else
        x_session_add_payload_to_channel(
                    X_OBJECT(_chan_),
                    _XS("98"), _XS("VP8"),
                    MEDIA_IO_MODE_OUT, &hints);
#endif
        // add control flow
        attr_list_clear(&hints);
        if(media)
        {
            x_session_add_payload_to_channel(
                        X_OBJECT(_chan_), _XS("101"),
                        _XS("HID"), MEDIA_IO_MODE_OUT, &hints);

            attr_list_clear(&hints);
        }
    }

#if 0
    if(media)
    {
        /* open/create hid channel */
        _chan_ = x_session_channel_open2(
                    X_OBJECT(_sess_), "m.H");

        setattr(_XS("mtype"), _XS("application"), &hints);
        _ASGN(X_OBJECT(_chan_), &hints);

        attr_list_clear(&hints);
        x_session_channel_set_transport_profile_ns(
                    X_OBJECT(_chan_), _XS("__icectl"),
                    _XS("jingle"), &hints);
        x_session_channel_set_media_profile_ns(
                    X_OBJECT(_chan_), _XS("__rtppldctl"),_XS("jingle"));
        x_session_add_payload_to_channel(
                    X_OBJECT(_chan_), _XS("101"), _XS("HID"), MEDIA_IO_MODE_OUT, &hints);
    }
#endif

    // commit channel
    setattr(_XS("$commit"), _XS("yes"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);
    attr_list_clear(&hints);

    x_session_start(X_OBJECT(_sess_));

    return true;
}


void
GobeeConnection::endCall(std::string peerjid, std::string sid)
{
    x_obj_attr_t hints =
    { 0, 0, 0 };

    TRACE("Terminating session '%s'\n",_AGET(o, "sid"));

    setattr("sid", const_cast<char *>(sid.c_str()), &hints);

    /* just call api */
    x_session_close(peerjid.c_str(),X_OBJECT(bus),&hints,0);
}
