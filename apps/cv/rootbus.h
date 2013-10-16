#ifndef ROOTBUS_H
#define ROOTBUS_H

#include <xwlib++/x_objxx.h>

class GbRootBus : public gobee::__x_object
{
    int __start_loop();

    static void *loop_worker(void *classprt);

public:
    GbRootBus();
    virtual int on_append(gobee::__x_object *parent);
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    virtual __x_object *operator+=(x_obj_attr_t *); // on_assign
    virtual int try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr);
    virtual void on_create();                   // on_create
};

#endif // ROOTBUS_H
