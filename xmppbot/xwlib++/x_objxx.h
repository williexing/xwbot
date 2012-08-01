/*
 * x_obj.hpp
 *
 *  Created on: Nov 2, 2011
 *      Author: artemka
 */

#ifndef X_OBJ_HPP_
#define X_OBJ_HPP_

#include <sys/types.h>

#include <xwlib/x_obj.h>

namespace gobee {

  class __path_listener
  {
  public:
    x_path_listener_t lstnr;
  };

/**
 * x_object C++ representation class
 * @ingroup XOBJECT
 */
class EXPORT_CLASS __x_object
{
public:
    x_object xobj;

public:
    __x_object();
    /**
     * Looks up objectclass in given namespace, allocates
     * space for it and initializes new object.
     *
     * @param ns Namespace name
     * @param cname Class name
     * @param size Atuomatic size value
     * @return Newly allocated object instance
     */
    void *operator new(size_t size,const char *ns,const char *cname);
    void operator delete(void *);

    __x_object *get_element_by_tag_name(const char *name);
    __x_object *get_child(const char *cname);
    __x_object *get_next();
    __x_object *get_sibling();
    __x_object *get_parent();
    const char *__get_name();
    void __set_name(const char *);
    void __setattr(const char *k, const char *v);
    const char *__getattr(const char *k);
    const char *operator[](const char *k);
    const char *__getenv(const char *k);
    __x_object *get_object_at_path(const char *p);
    __x_object *get_bus();
    bool add_child(__x_object *child);
    void write_content(const char *bud, unsigned int len);

    //////////////////////////////////////////
    /////  VTABLE USED ALSO FROM C FILES /////
    ///  do NOT modify function list order ///
    //////////////////////////////////////////
    virtual int equals(x_obj_attr_t *);         // match
    /* virtual constructor */
    virtual void on_create();                   // on_create
    virtual int on_append(__x_object *parent); // on_append
    virtual void on_remove();                   // on_remove
    virtual int on_child_append(__x_object *child); // on_child_append
    virtual void on_child_remove(__x_object *child);  // on_child_remove
    virtual void on_release();                  // on_release
    virtual __x_object *operator+=(x_obj_attr_t *); // on_assign
    virtual void finalize();                    // finalize
    virtual int classmatch(x_obj_attr_t *);
    virtual __x_object *tx(__x_object *o, x_obj_attr_t *);        // tx
    virtual __x_object &operator>>(__x_object *o);        // rx
    virtual int try_write (void *buf, u_int32_t len, x_obj_attr_t *attr); // try_write
    virtual int try_writev (const struct iovec *iov, int iovcnt,
        x_obj_attr_t *attr);
    //////////////////////////////////////////
    //////////////////////////////////////////
};


class command {

};

/**
 * C++ wrapper over Xwlib
 * @ingroup XOBJECT
 */
EXPORT_SYMBOL int __x_class_register_xx(gobee::__x_object *protoobj,unsigned int size,
    const char *ns, const char *cname);

}

#endif /* X_OBJ_HPP_ */
