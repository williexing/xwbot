#include "gobeemsg.h"
#include <plugins/sessions_api/sessions.h>

extern int registerMessageIODeviceWithSessionId(const char *jid, GobeeMsg *dev);

int
GobeeMsg::chekNexthop()
{
    int err = -1;
    gobee::__x_object *nxthop;
    gobee::__x_object *parent = this->get_parent();

    ::printf("[GobeeMsg] %s()\n",__FUNCTION__);

    if (!this->xobj.write_handler &&
            parent)
    {
        nxthop = this->get_parent()->get_child(MEDIA_OUT_CLASS_STR);
        x_object_set_write_handler((x_object *)this, (x_object *)nxthop);
    }

    if (this->xobj.write_handler)
        err = 0;

    return err;
}

void
GobeeMsg::sendMessage(std::string &msg)
{
    int err = 0;
    ::printf("[GobeeMsg] %s()\n",__FUNCTION__);

    if (this->xobj.write_handler
            || !this->chekNexthop())
    {
        err = _WRITE(this->xobj.write_handler,
                     (void *)(msg.c_str()), msg.size(), NULL);
    }

    return;
}


int
GobeeMsg::on_append(gobee::__x_object *parent)
{
    ::printf("[GobeeMsg] %s()\n",__FUNCTION__);
    this->__setattr("Descr","Qt Message Dialog");
    this->__setattr("Vendor","Gobee Alliance, Inc.");
    registerMessageIODeviceWithSessionId(parent->__getenv("sid"), this);
    return 0;
}

int
GobeeMsg::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    ::printf("[GobeeMsg] %s()\n",__FUNCTION__);
//    TRACE("received message to write %s\n", buf);
    ::printf("[GobeeMsg] received message to write %s\n",buf);
    return err;
}

GobeeMsg::GobeeMsg()
{
    ::printf("[GobeeMsg] %s()\n",__FUNCTION__);
    gobee::__x_class_register_xx(this, sizeof(GobeeMsg),
                                 "gobee:media", "__ibshell");
}

static GobeeMsg msgproto;
