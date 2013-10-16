//
//  SessionCtl.h
//  gobee
//
//  Created by CrossLine Media, LLC on 19.02.13.
//
//

#ifndef __gobee__SessionCtl__
#define __gobee__SessionCtl__

#include <xwlib++/x_objxx.h>

class MediaSessionController : public gobee::__x_object
{

public:
    MediaSessionController();
    virtual int on_append(gobee::__x_object *parent); // on_append
    virtual void on_remove();                   // on_remove
    virtual void  on_tree_destroy();
    virtual int try_write (void *buf, u_int32_t len, x_obj_attr_t *attr); // try_write
    virtual int try_writev (const struct iovec *iov, int iovcnt,
                            x_obj_attr_t *attr);
};

#endif /* defined(__gobee__SessionCtl__) */
