//
//  gobee_ios_vhid.c
//  GobeePhone
//
//  Created by артемка on 06.04.13.
//  Copyright (c) 2013 CrossLine Media, LLC. All rights reserved.
//

#include <x_config.h>

#define DEBUG_PRFX "[ IOSHIDDEV ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <stdio.h>

#define X_IN_DEVDIR "__proc/out"
#define HID_VENDOR_ID "GobeeHid_4ios"

int register_hid_for_Sid(const char *sid, x_object *dev);

enum
{
  HID_MOUSE_EVENT = 0x1,
  HID_KEY_EVENT,
  HID_SCROLL_EVENT,
};

typedef struct _ios_hid
{
  x_object xobj;
} ios_hid;

static int
ios_hid_on_append(x_object *thiz, x_object *parent)
{
  ENTER;
  TRACE("new HIDDEV sid=%s\n",_ENV(thiz,"sid"));
  _ASET(thiz,"Descr","Anios HID event generator");
  _ASET(thiz,"Vendor","Gobee Alliance, Inc.");
  //    register_hid_for_Sid(_ENV(_PARNT(thiz),"sid"), thiz);
  register_hid_for_Sid(_ENV(thiz,"sid"), thiz);
  return 0;
}

static __inline__ void
sendMouseEvent(ios_hid *thiz, int *_dat)
{
  x_object *tmp;
  char buf[64];
  
  TRACE("[GobeeHid] sendMouseEvent()1: this(%p) {%d::%d:%dx%d} EXIT\n",
        thiz,_dat[0],_dat[1],_dat[2],_dat[3]);
  
  if (!thiz->xobj.write_handler)
  {
    tmp = _CHLD(_PARNT(thiz),"out$media");
    if (!tmp)
    {
      
      TRACE("No output channel... Decreasing hid status...\n");
      ERROR;
      return;
    }
    
    tmp = _CHLD(tmp,"101");
    if (!tmp)
    {
      TRACE("No output channel... Decreasing hid status...\n");
      ERROR;
      return;
    }
    else
    {
      thiz->xobj.write_handler = X_OBJECT(tmp);
      _REFGET(thiz->xobj.write_handler);
    }
  }
  else
  {
    TRACE("Handler OK\n");
  }
  
  //    TRACE("[GobeeHid] sendMouseEvent()2: {%d::%d:%dx%d} EXIT\n",
  //             _dat[0],_dat[1],_dat[2],_dat[3]);
  //
  if (thiz->xobj.write_handler)
  {
    TRACE("[GobeeHid] sendMouseEvent()2: %s, {%d::%d:%dx%d} EXIT\n",
          _GETNM(thiz->xobj.write_handler),
          _dat[0],_dat[1],_dat[2],_dat[3]);
    _WRITE(thiz->xobj.write_handler, _dat, 4*sizeof(int), NULL);
  }
  else
  {
    TRACE("[GobeeHid] ERROR!! sendMouseEvent()2: {%d::%d:%dx%d} EXIT\n",
          _dat[0],_dat[1],_dat[2],_dat[3]);
  }
  
  TRACE("[GobeeHid] sendMouseEvent()3: {%d::%d:%dx%d} EXIT\n",
        _dat[0],_dat[1],_dat[2],_dat[3]);
}

void
n_emit_hid_event
(x_object *thiz,
 int tip, int x, int y, int buttons)
{
  ios_hid *hiddev;
  int _dat[4];
  
  TRACE("[GobeeHid] n_emit_hid_event(%u,%d,%dx%d,%d}\n",
        tip,x,y,buttons);
  
  if (!thiz)
    return;
  
  hiddev = (ios_hid *)(void *)thiz;
  _dat[0] = tip;
  _dat[1] = x;
  _dat[2] = y;
  _dat[3] = buttons;
  TRACE("[ xwjni-touch ]::[GobeeHid] n_emit_hid_event()2: {%d::%d:%dx%d}\n",
        tip,x,y,buttons);
  sendMouseEvent(hiddev,_dat);
}

static struct xobjectclass ios_hid_clazz;


static void
___on_x_path_event(x_object *obj)
{
  x_object_add_path(X_OBJECT(obj), X_IN_DEVDIR "/" HID_VENDOR_ID, NULL);
}

static struct x_path_listener _bus_listener;

__x_plugin_visibility
__plugin_init void
ios_hid_init(void)
{
  ios_hid_clazz.cname = _XS("hidmux");
  ios_hid_clazz.on_append = &ios_hid_on_append;
  
  x_class_register_ns(ios_hid_clazz.cname,
                      &ios_hid_clazz,_XS("gobee:media"));
  
  _bus_listener.on_x_path_event = (void(*)(void*))&___on_x_path_event;
  x_path_listener_add("/", (x_path_listener_t *) (void *) &_bus_listener);
  
  return;
}

PLUGIN_INIT(ios_hid_init);
