#ifndef GOBEEHID_H
#define GOBEEHID_H

    #include <QtGui>
#include <QtCore>

#include <xwlib++/x_objxx.h>

#define X_IN_DEVDIR "__proc/out"
#define HID_VENDOR_ID "GobeeHid_QVFB"

enum
{
    HID_MOUSE_EVENT = 0x1,
    HID_KEY_EVENT,
    HID_SCROLL_EVENT
};

class GobeeHid : public gobee::__x_object
{
public:
    GobeeHid();
public:
  virtual int on_append(gobee::__x_object *parent);
  virtual void on_remove();
    //    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    void sendMouseEvent(QMouseEvent *event);
    void sendKeyboardEvent(QKeyEvent *event);
    void sendTelephonyEvent();

private:
    int lastX0;
    int lastY0;
    int buttons;
};

#endif // GOBEEHID_H
