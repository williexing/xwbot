//
//  SessionCtl.cpp
//  gobee
//
//  Created by CrossLine Media, LLC on 19.02.13.
//
//

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xcmdapi++] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include "SessionCtl.h"

int
MediaSessionController::on_append(gobee::__x_object *parent)
{
    return 0;
}

void
MediaSessionController::on_remove()
{
    
}

void
MediaSessionController::on_tree_destroy()
{
    
}

int
MediaSessionController::try_write (void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int buftype;

    /* main switch (dispatcher) */
    switch (buftype)
    {

    };

    return len;
}

int
MediaSessionController::try_writev (const struct iovec *iov, int iovcnt,
                x_obj_attr_t *attr)
{
    return 1;
}



////////////////////////////////////////////////////////////////////////////////////
static void
___on_x_path_event(gobee::__x_object *obj)
{
    gobee::__x_object *_o =
            new ("http://jabber.org/protocol/commands", "RtcMediaController") MediaSessionController();
    ENTER;
    
    _o->__set_name("RtcMediaController");
    _o->__setattr("name", "RTC Media Session Controller");
    
    EXIT;
}

static gobee::__path_listener mediaCtl_BusListener;


MediaSessionController::MediaSessionController()
{
    gobee::__x_class_register_xx(this, sizeof(MediaSessionController),
                                 "http://jabber.org/protocol/commands", "RtcMediaController");
    
    mediaCtl_BusListener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
//    x_path_listener_add("/", (x_path_listener_t *) (void *) &mediaCtl_BusListener);
    
    return;
}

static MediaSessionController oproto;


