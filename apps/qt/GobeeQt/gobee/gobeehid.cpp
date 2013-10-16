#include <iostream>


#include <x_config.h>
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <stdio.h>

#include <string>
#include <map>

#include "gobeehid.h"

extern int registerHidDeviceWithSessionId(const char *jid, GobeeHid *dev);

int
GobeeHid::on_append(gobee::__x_object *parent)
{
    this->__setattr("Descr","Qt HID event generator");
    this->__setattr("Vendor","Gobee Alliance, Inc.");
    registerHidDeviceWithSessionId(parent->__getenv("sid"), this);
    return 0;
}

void
GobeeHid::on_remove()
{
  
}

void
GobeeHid::sendMouseEvent(QMouseEvent *event)
{
    gobee::__x_object *tmp;
    char buf[64];
    int _dat[4]; // type,buttons,x,y

    if (!this->xobj.write_handler)
    {
        ::printf("[GobeeHid] %d\n",__LINE__);

      gobee::__x_object *parent =  this->get_parent();
      if (!parent)
      {
        ::printf("[GobeeHid] %d\n",__LINE__);
        
        ::printf("Seems we are not in the tree... Decreasing hiddev status...\n");
        ERROR;
        return;
      }

      
        tmp = parent->get_child("out$media");
        ::printf("[GobeeHid] %d\n",__LINE__);
        if (!tmp)
        {
            ::printf("[GobeeHid] %d\n",__LINE__);

            ::printf("No output channel... Decreasing hiddev status...\n");
            ERROR;
            return;
        }

        /**
         * @todo "Remove hardcoded values from here"
         * @bug Hardcoded value
         */
        tmp = tmp->get_child("101");
        ::printf("[GobeeHid] %d\n",__LINE__);
        if (!tmp)
        {
            ::printf("[GobeeHid] %d\n",__LINE__);
            ::printf("No output channel... Decreasing camera status...\n");
            ERROR;
            return;
        }
        else
        {
            ::printf("[GobeeHid] %d\n",__LINE__);
            x_object_set_write_handler(X_OBJECT(this),X_OBJECT(tmp));
        }
    }
    else
    {
        ::printf("[GobeeHid] %d\n",__LINE__);
        TRACE("Handler OK\n");
    }

    ::printf("[GobeeHid] %d\n",__LINE__);
    _dat[0] = HID_MOUSE_EVENT;
    _dat[1] = event->pos().x();
    _dat[2] = event->pos().y();
    _dat[3] = (int)event->buttons();
    ::printf("[GobeeHid] %d\n",__LINE__);

    _WRITE(this->xobj.write_handler, &_dat, sizeof(_dat), NULL);
    ::printf("[GobeeHid] sendMouseEvent(): {%d::%d:%dx%d} EXIT\n",
             _dat[0],_dat[1],_dat[2],_dat[3]);
}

void
GobeeHid::sendKeyboardEvent(QKeyEvent *event)
{
    gobee::__x_object *tmp;
//    char buf[64];
    int _dat[4];

    std::cout << "[GobeeHid] keyPressEvent() ENTER\n";

    if (!this->xobj.write_handler)
    {

        tmp = this->get_parent()->get_child("out$media");
        if (!tmp)
        {
            TRACE("No output channel... Decreasing camera status...\n");
            ERROR;
            return;
        }

        tmp = tmp->get_child(NULL);
        if (!tmp)
        {
            TRACE("No output channel... Decreasing camera status...\n");
            ERROR;
            return;
        }
        else
        {
            this->xobj.write_handler = X_OBJECT(tmp);
            _REFGET(X_OBJECT(this->xobj.write_handler));
        }
    }
    else
    {
        TRACE("Handler OK\n");
    }

    _dat[0] = HID_KEY_EVENT;
    _dat[1] = 0x20;
    _dat[2] = 0x30;
    _WRITE(this->xobj.write_handler, &_dat, sizeof(_dat), NULL);
    std::cout << "[GobeeHid] keyPressEvent() EXIT\n";
}

void
GobeeHid::sendTelephonyEvent()
{
    char buf[64];
    int len = 0;
    _WRITE(this->xobj.write_handler, buf, len, NULL);
}

static void
___on_x_path_event(gobee::__x_object *obj)
{
    x_object_add_path(X_OBJECT(obj), X_IN_DEVDIR "/" HID_VENDOR_ID, NULL);
}

static gobee::__path_listener _bus_listener;

GobeeHid::GobeeHid()
{
    gobee::__x_class_register_xx(this, sizeof(GobeeHid),
                                 "gobee:media", "hidmux");

    _bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("/", (x_path_listener_t *) (void *) &_bus_listener);

    lastX0 = -1;
    lastY0 = -1;

    return;
}

static GobeeHid hidproto;
