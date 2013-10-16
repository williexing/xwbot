/*
 * x_obj.cpp
 *
 *  Created on: Nov 2, 2011
 *      Author: artemka
 */
#include <string.h>
#include <stdio.h>

#undef DEBUG_PRFX

#include <x_config.h>
#if TRACE_XOBJECTXX_ON
#define DEBUG_PRFX "[xwlib++] "
#endif

#include <sys/types.h>
#include <xwlib/x_obj.h>
#include "x_objxx.h"

extern "C" int
x_class_register(const char *, x_objectclass *oclass);

typedef int
(*fn_t)();

int
gobee::__x_class_register_xx(gobee::__x_object *protoobj, unsigned int size,
    const char *ns, const char *cname)
{
  size_t siz;
  fn_t *vptr = *(fn_t **) protoobj;
  x_objectclass *oclass = new x_objectclass;

  oclass->cname = (char *) cname;
  oclass->psize = size - sizeof(x_object);

  siz = (size_t) (&((x_objectclass*) 0)->lentry);

  // make a copy of vptr to objclass
  memcpy((void *) &oclass->match, (void *) vptr, siz);

  int ret = x_class_register_ns(cname, oclass, ns);

  return ret;
}

void *
gobee::__x_object::operator new(size_t size, const char *ns, const char *cname)
{
  gobee::__x_object *ret = (gobee::__x_object *) (void *) x_object_new_ns(
      _XS(cname), _XS(ns));
  TRACE(">> %p, %p=%s\n", ret, &ret->xobj.entry.name, ret->xobj.entry.name);
  return (void *) ret;
}

void
gobee::__x_object::operator delete(void *ptr)
{
  _REFPUT(X_OBJECT(ptr),NULL);
}

int
gobee::__x_object::equals(x_obj_attr_t *attr)
{
  ENTER;
  EXIT;
  return 0; // false by default
}

int
gobee::__x_object::classmatch(x_obj_attr_t *attr)
{
  ENTER;
  EXIT;
  return 0; // false by default
}

void
gobee::__x_object::on_create()
{
  ENTER;
  EXIT;
}

int
gobee::__x_object::on_append(__x_object *parent)
{
  TRACE(">>> %p, %p\n",parent,this);
  return 0;
}

void
gobee::__x_object::on_remove()
{
  ENTER;
  EXIT;
}

void
gobee::__x_object::on_tree_destroy()
{
    ENTER;
    EXIT;
}

int
gobee::__x_object::on_child_append(__x_object *child)
{
  ENTER;
  EXIT;
  return 0;
}

void
gobee::__x_object::on_child_remove(__x_object *child)
{
  ENTER;
  EXIT;
}

gobee::__x_object::__x_object()
{
  //  ENTER;
  TRACE(">> %p, %p=%s\n", this, &this->xobj.entry.name, this->xobj.entry.name);
  //  EXIT;
}

void
gobee::__x_object::on_release()
{
  //  ENTER;
  TRACE(">> %p, %p=%s\n", this, &this->xobj.entry.name, this->xobj.entry.name);
  //  EXIT;
}

gobee::__x_object *
gobee::__x_object::operator+=(x_obj_attr_t *attr)
{
  ENTER;
  x_object_default_assign_cb(X_OBJECT(this), attr);
  EXIT;
  return this;
}

void
gobee::__x_object::commit()
{
  ENTER;
  // FIXME This call should be done for C objects only to prevent recusrive call
    this->xobj.objclass->commit(X_OBJECT(this));
  EXIT;
}


void
gobee::__x_object::on_suspend()
{
    ENTER;
    // FIXME This call should be done for C objects only to prevent recusrive call
      this->xobj.objclass->on_suspend(X_OBJECT(this));
    EXIT;
}

void
gobee::__x_object::on_resume()
{
    ENTER;
    // FIXME This call should be done for C objects only to prevent recusrive call
      this->xobj.objclass->on_suspend(X_OBJECT(this));
    EXIT;
}


int
gobee::__x_object::tx(gobee::__x_object *o, x_obj_attr_t *ctx)
{
  ENTER;
  x_object_send_down(X_OBJECT(this->get_parent()), X_OBJECT(o), ctx);
  EXIT;
  return 0;
}

int
gobee::__x_object::rx(/*gobee::__x_object *to,*/ const gobee::__x_object *from,
       const gobee::__x_object *msg)
{
  return 0;
}

int
gobee::__x_object::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
  return -1;
}

int
gobee::__x_object::try_writev(const struct iovec *iov, int iovcnt,
    x_obj_attr_t *attr)
{
  return -1;
}

int
gobee::__x_object::try_writev2 (const struct iovec *iov, int slot, int iovcnt,
    x_obj_attr_t *attr)
{
    return -1;
}

int
gobee::__x_object::try_read(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    return -1;
}

int
gobee::__x_object::try_readv(const struct iovec *iov, int iovcnt,
    x_obj_attr_t *attr)
{
    return -1;
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
gobee::__x_object *
gobee::__x_object::get_element_by_tag_name(const char *name)
{
  return (gobee::__x_object *) (void *) x_object_get_element_by_tag_name(
      (const x_object *) (void *) this, _XS(name));
}

gobee::__x_object *
gobee::__x_object::get_child(const char *name)
{
  return (gobee::__x_object *) (void *) x_object_get_child(
      (x_object *) (void *) this, _XS(name));
}

gobee::__x_object *
gobee::__x_object::get_next()
{
  return (gobee::__x_object *) (void *) x_object_get_next(X_OBJECT(this));
}

gobee::__x_object *
gobee::__x_object::get_sibling()
{
  return (gobee::__x_object *) (void *) x_object_get_sibling(X_OBJECT(this));
}

gobee::__x_object *
gobee::__x_object::get_parent()
{
  return (gobee::__x_object *) (void *) x_object_get_parent(X_OBJECT(this));
}

const char *
gobee::__x_object::__get_name()
{
  // return x_object_get_name(X_OBJECT(this));
  return this->xobj.entry.name;
}

void
gobee::__x_object::__set_name(const char *name)
{
  x_object_set_name(X_OBJECT(this), _XS(name));
}

void
gobee::__x_object::write_content(const char *buf, unsigned int len)
{
  x_object_set_content(X_OBJECT(this),buf,len);
}

void
gobee::__x_object::__setattr(const char *k, const char *v)
{
  x_object_setattr(X_OBJECT(this), _XS(k), _XS(v));
}

const char *
gobee::__x_object::__getattr(const char *k)
{
  const char *res;
  res = x_object_getattr(X_OBJECT(this), _XS(k));
  TRACE("%p,k(%p),v(%p)=%s\n",(void *)this,(void *)k,(void *)res, res);
  return res;
}

const char *
gobee::__x_object::operator[](const char *k)
{
  ENTER;
  EXIT;
  return x_object_getattr(X_OBJECT(this),_XS(k));
}

/*
 gobee::__x_object *
 gobee::__x_object::operator[](const char *k)
 {
 ENTER;
 EXIT;
 return this->get_child(k);
 }
 */

const char *
gobee::__x_object::__getenv(const char *k)
{
  return x_object_getenv(X_OBJECT(this), _XS(k));
}

gobee::__x_object *
gobee::__x_object::get_object_at_path(const char *p)
{
  gobee::__x_object *o = (gobee::__x_object *) (void *) x_object_from_path(
      X_OBJECT(this), _XS(p));
  return o;
}

gobee::__x_object *
gobee::__x_object::get_bus()
{
  gobee::__x_object *o = (gobee::__x_object *) (void *) (this->xobj.bus);
  return o;
}

bool
gobee::__x_object::add_child(gobee::__x_object *child)
{
  bool result;
  ENTER;
  result = (bool) x_object_append_child(X_OBJECT(this), X_OBJECT(child));
  EXIT;
  return result;
}

gobee::__x_object *
gobee::__x_object::operator[](int id)
{
  ENTER;
  EXIT;
  return (gobee::__x_object *) (void *) x_object_get_child_from_index(X_OBJECT(this),id);
}

int
gobee::__x_object::write_out (const void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    return x_object_write_out(X_OBJECT(this), buf, len, attr);
}

int
gobee::__x_object::writev_out (const struct iovec *iov, int iovcnt,
                    x_obj_attr_t *attr)
{
    return x_object_writev_out(X_OBJECT(this), iov, iovcnt, attr);
}

