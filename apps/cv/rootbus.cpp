#undef DEBUG_PRFX

#include <x_config.h>

#define DEBUG_PRFX "[cv-bus] "

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <ev.h>

#include "rootbus.h"

void *
GbRootBus::loop_worker(void *classprt)
{
//    loop = ev_loop_new(EVFLAG_AUTO);
    GbRootBus *bus = static_cast<GbRootBus *>(classprt);

    TRACE("Starting new loop\n");

    for(;;)
    {
        ev_loop((struct ev_loop *)bus->xobj.loop,0);
        usleep(100000);
    }

    TRACE("Ending new loop\n");
    return NULL;
}

int
GbRootBus::__start_loop()
{
    if (!this->xobj.loop)
    {
        TRACE("Starting new loop\n");
        this->xobj.loop = ev_loop_new(EVFLAG_AUTO);
    }
    else
    {
        TRACE("Loop already started\n");
    }

    if(
            x_thread_run(&this->xobj.looptid,
                         &GbRootBus::loop_worker,
                         (void *) this) != 0
            )
    {
        TRACE("Thread not created\n");
        this->xobj.cr &= ~(1 << 1);
    }

    TRACE("Thread start ok\n");
}

int
GbRootBus::on_append(gobee::__x_object *parent)
{

}

int
GbRootBus::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{

}

gobee::__x_object *
GbRootBus::operator+=(x_obj_attr_t *)
{

}

void
GbRootBus::on_create()
{
    TRACE("Who is on duty today?\n");
    __start_loop();
}

int
GbRootBus::try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr)
{

}

GbRootBus::GbRootBus()
{
    gobee::__x_class_register_xx(this, sizeof(GbRootBus),
                                 "gobee","#rootbus");
    TRACE("Registered!\n");
    return;
}

static GbRootBus oproto;
