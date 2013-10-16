
#include <x_config.h>

#define DEBUG_PRFX "[ V-SOUND ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <stdio.h>

#define X_OUT_DEVDIR "__proc/out"
#define RECOGNIZER_VENDOR_ID "QtSound_CURL"

#include "vsiri.h"

#define ALSA_BRATE 16000

int
GobeeSiri::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    ENTER;
    EXIT;
    return len;
}

int
GobeeSiri::on_append(gobee::__x_object *parent)
{
    ENTER;
    EXIT;
    return 0;
}

void
GobeeSiri::on_release()
{
    ENTER;
    EXIT;
}

//void
//GobeeSiri::on_remove()
//{
//    ENTER;
//    EXIT;
//}

//int
//GobeeSiri::equals(x_obj_attr_t *a)
//{
//    return (int) true;
//}

//void
//GobeeSiri::on_create()
//{
//}

//void
//GobeeSiri::finalize()
//{

//}

//void
//GobeeSiri::on_tree_destroy()
//{
//    ENTER;
//    EXIT;
//}

gobee::__x_object *
GobeeSiri::operator+=(x_obj_attr_t *attrs)
{
    ENTER;
    EXIT;
    return this;
}


static void
___on_x_path_event(gobee::__x_object *obj)
{
    x_object_add_path(X_OBJECT(obj),
                      X_OUT_DEVDIR "/" RECOGNIZER_VENDOR_ID, NULL);
}

static gobee::__path_listener _recogn_bus_listener;

GobeeSiri::GobeeSiri()
{
    gobee::__x_class_register_xx(this, sizeof(GobeeSiri),
                                 "gobee:media", "speech_recognizer");

    _recogn_bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("/", (x_path_listener_t *) (void *) &_recogn_bus_listener);

    TRACE("Ok!!!\n");
    return;
}

static GobeeSiri oproto;
