#ifndef GOBEEMSG_H
#define GOBEEMSG_H

#include <QtGui>
#include <QtCore>

#include <xwlib++/x_objxx.h>

#define X_IN_DEVDIR "__proc/out"
#define HID_VENDOR_ID "GobeeMsg"

/**
 *
 *            +--------+
 *            |   UI   |
 *            +--------+
 *               /|\
 *                |
 *               \|/
 *          +------------+
 *  in$ --->| __ibshell  |----> out$
 *          +------------+
 */

class GobeeMsg : public gobee::__x_object
{
public:
    GobeeMsg();
    void sendMessage(std::string &msg);
private:  int chekNexthop();
public:
    virtual int on_append(gobee::__x_object *parent);
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
};

#endif // GOBEEMSG_H
