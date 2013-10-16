/*
 * Copyright (c) 2011, artemka
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * xwbot
 * Created on: Jun 6, 2011
 *     Author: artemka
 *
 */

#ifndef X_OBJ_H_
#define X_OBJ_H_

#include "x_types.h"
#include "x_ref.h"
#include "x_string.h"

#define XOBJ_CS_NAME _cs
#define XOBJ_CS_INIT(x) CS_INIT(CS2CS(x->XOBJ_CS_NAME))
#define XOBJ_CS_RELEASE(x) \
  do { \
    if (x->csready) \
    { \
      CS_RELEASE(CS2CS(x->XOBJ_CS_NAME)); \
      x->csready = 0; \
      } \
  } while (0)

#define XOBJ_CS_LOCK(x) \
  do { \
    if (!x->csready) \
    { \
        XOBJ_CS_INIT(x); \
        x->csready = 1; \
    } \
    CS_LOCK(CS2CS(x->XOBJ_CS_NAME)); \
  } while (0)

#define XOBJ_CS_UNLOCK(x) \
  do { \
    if (x->csready) \
      { CS_UNLOCK(CS2CS(x->XOBJ_CS_NAME)); } \
  } while (0)

#ifdef __cplusplus
extern "C"
  {
#endif

/* forward declarations */
struct xobject;
struct xobject_factory;
struct xobjectclass;

#define TYPE_OBJECT_NODE        1
#define TYPE_ALIAS_NODE         2

typedef struct tree_node
{
  int type;
  char *name;
  int child_count;
  /* OBJECT RELATIONS */
  struct tree_node *parent;
  struct tree_node *next;
  struct tree_node *sibling;
  struct tree_node *sons;
} tree_node_t;

typedef struct xalias
{
#ifndef __cplusplus
  void *__vptr;
#endif
  tree_node_t *target;
  tree_node_t entry;
} x_alias;

enum
{
  EVT_CHILD_APPEND = 0, EVT_CHILD_REMOVE, EVT_ASSIGN, EVT_STATE,
};

/**
 * @ingroup XOBJECT
 */
typedef struct ev_loop xevent_dom_t;

#define DEFAULT_IOLIFECOUNT 100

typedef struct xobject
{
#ifndef __cplusplus
  void *__vptr;
#endif /* __cplusplus */
  struct ht_cell attrs;
  x_string content;
  struct xobject *bus;

  /** event domain object of this object */
  struct xobject *event_domain;

#ifdef XOBJECT_EVENTS
  xevent_dom_t *loop;
  THREAD_T looptid;
#endif

  uint32_t cr; // control register
  int io_life_count;
  struct xref refcount;
  struct xobjectclass *objclass;
  tree_node_t entry; /* this must always be at the beginning */

  /** this provides namespace relation */
  struct xobject_factory *factory;
  unsigned int extra;

  /** counts number of assignments */
  unsigned int assign_count;

  /** write handler support fields */
  struct xobject *write_handler;
    
  /** writeout vector - dynamic vector of all write handlers */
  x_obj_listener_t writeout_vector;

  /** instance listeners */
  x_obj_listener_t listeners;
  /* END of write handler support fields */

  /** critical section declaration */
  CS_DECL(XOBJ_CS_NAME);
  int csready;
} x_object;

/**
 * @ingroup XOBJECT
 */
typedef struct xobjectclass
{
  /* Object matching function */BOOL
  (*match)(x_object *o, x_obj_attr_t *);
  /* constructor */
  void (*on_create)(x_object *o);
  /* handler of append event (fires after appending to parent) */
  int  (*on_append)(x_object *o, x_object *parent);
  /* handler of removing event (appears when object is removing from its parent) */
  void  (*on_remove)(x_object *o);
  /** handler of event in which object's tree is tried be destroyed */
  void  (*on_tree_destroy)(x_object *o);
  /**  */
  void  (*on_child_append)(x_object *o, x_object *child);
  /**  */
  void  (*on_child_remove)(x_object *o, x_object *child);
  /* default destructor */
  void  (*on_release)(x_object *o);
  /**
   *  called when parser enters an object
   *  This callback is fired by x_bus only when object is
   *  already appended to its parent.
   */
  x_object *  (*on_assign)(x_object *o, x_obj_attr_t *attr);

  /* called when parser exits an object */
  void  (*commit)(x_object *o);

  void (*on_suspend)(x_object *o);
  void (*on_resume)(x_object *o);

  /**
   *  Class matching func
   *  @param to This to match
   *  @param attrs Attribute to match
   */
  int  (*classmatch)(x_object *to, x_obj_attr_t *attr);
  /** transmit object downstream */
  int  (*tx)(x_object *to, x_object *o, x_obj_attr_t *);
  /** send message to object */
  int  (*rx)(x_object *to, const x_object *from, const x_object *o);
  /** try write raw data */
  int  (*try_write)(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr);
  /** try write vector raw data */
  int  (*try_writev)(x_object *o, const x_iobuf_t *iov, u_int32_t iovcnt,
      x_obj_attr_t *attr);

  /** write data to slot */
  int  (*try_writev2)(x_object *o, int slot, const x_iobuf_t *iov, u_int32_t iovcnt,
      x_obj_attr_t *attr);

  /** try write raw data */
  int  (*try_read)(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr);
  /** try write vector raw data */
  int  (*try_readv)(x_object *o, const struct iovec *iov, int iovcnt,
      x_obj_attr_t *attr);

#if 0
  /**
   * Fires on subtree event
   * @param sender - source object
   * @param o - destination object
   * @param flags - event mask
   * @param distance - distance from sender to destination.
   * @return FALSE - to continue, TRUE - to stop event passing.
   */
  int (*on_subtree_event)(x_object *o, x_object *sender, u_int32_t flags,
      int distance);

  /**
   * Fires on instance specific events.
   */
  int (*on_instance_event)(x_object *o, x_object *sender, u_int32_t flags);
#endif

  /*--------------------------------*/
  /*--------------------------------*/
  list_entry_t lentry;
  /* classname */
  char *cname;
  /* namespace */
  char *ns;
  /* reserved space space of x_object (payload size) */
  u_int32_t psize;
} x_objectclass;

/**
 * @ingroup XOBJECT
 */
X_DEPRECATED EXPORT_SYMBOL void x_object_set_write_handler(x_object *o, x_object *h);

/**
 * @ingroup XOBJECT
 *
 * @{
 */
#define XOBJ_CR_DIRTY 0x1
/** @} */


/**
 * @defgroup XOBJECT Xobject (xwlib) core API
 *
 * @todo Add new function x_object_reparent() to do safe reparent operations
 *
 * @{
 */
#define SYNAPTIC_API
X_DEPRECATED EXPORT_SYMBOL int x_object_write_out(const x_object *from, const void *buf, int len, x_obj_attr_t *attr);
EXPORT_SYMBOL int x_object_writev_out(const x_object *from, const struct iovec *iov, int iovcnt,
                                            x_obj_attr_t *attr);
EXPORT_SYMBOL int x_object_add_follower(x_object *thiz, x_object *follower, int slot);
EXPORT_SYMBOL int x_object_del_follower(x_object *thiz, x_object *follower);

X_DEPRECATED EXPORT_SYMBOL int x_object_try_write(x_object *xobj, void *buf, int len, x_obj_attr_t *attr);
X_DEPRECATED EXPORT_SYMBOL int x_object_try_writev(x_object *xobj, const struct iovec *iov, int iovcnt,
    x_obj_attr_t *attr);
EXPORT_SYMBOL int x_object_try_writev2(x_object *xobj, int slot, const struct iovec *iov, int iovcnt, x_obj_attr_t *attrs);

EXPORT_SYMBOL x_object *x_object_touch(x_object *xobj);
EXPORT_SYMBOL x_object *x_object_add_path(x_object *ctx, const x_string_t path, x_object *obj);
EXPORT_SYMBOL x_object *x_object_from_path(const x_object *ctx, const x_string_t path);
EXPORT_SYMBOL x_object *x_object_mkpath(const x_object *ctx, const x_string_t path);
EXPORT_SYMBOL void x_object_print_path(x_object *obj, int depth);
EXPORT_SYMBOL int x_object_to_path(x_object *obj, x_string_t pathbuf, unsigned int bufsiz);
EXPORT_SYMBOL x_object *x_object_get_element_by_tag_name(const x_object *parent, const x_string_t name);
EXPORT_SYMBOL x_object *x_object_get_child(x_object *xparent, const x_string_t cname);
EXPORT_SYMBOL x_object *x_object_get_child_from_index(x_object *xparent, int ind);
EXPORT_SYMBOL int x_object_get_index(x_object *xobj);
EXPORT_SYMBOL x_object *x_object_get_next(const x_object *xobj);
EXPORT_SYMBOL x_object *x_object_get_sibling(const x_object *xobj);
EXPORT_SYMBOL x_object *x_object_get_parent(const x_object *xobj);
EXPORT_SYMBOL const x_string_t x_object_get_name(const x_object *xobj);
EXPORT_SYMBOL void x_object_set_name(x_object *xobj, const x_string_t name);
EXPORT_SYMBOL int x_object_set_content(x_object *xobj, const char *buf, u_int32_t len);
EXPORT_SYMBOL void x_object_init(x_object *xobj);
EXPORT_SYMBOL x_alias *x_object_make_alias(x_object *xobj, x_object *parent,
    const x_string_t alias_name);
EXPORT_SYMBOL void x_object_setattr(x_object *xobj, const x_string_t name, const x_string_t val);
EXPORT_SYMBOL x_string_t x_object_getattr(x_object *xobj, const x_string_t name);
EXPORT_SYMBOL int x_object_append_child(x_object *parent, x_object *xobj);
EXPORT_SYMBOL int x_object_append_child_no_cb(x_object *parent, x_object *child);
EXPORT_SYMBOL void x_object_remove_from_parent(x_object *xobj);
EXPORT_SYMBOL void x_object_destroy(x_object *xobj);
EXPORT_SYMBOL int x_object_destroy_no_cb(x_object *xobj);
EXPORT_SYMBOL x_object *x_object_clone(x_object *xibj);
EXPORT_SYMBOL x_object *x_object_copy(x_object *xobj);
EXPORT_SYMBOL x_object *x_object_full_copy(x_object *o);
EXPORT_SYMBOL const x_string_t x_object_getenv(x_object *xobj, const x_string_t anam);
EXPORT_SYMBOL int x_object_match(x_object *o, x_obj_attr_t *attrs);
EXPORT_SYMBOL x_object *x_object_find(x_object *starto, x_obj_attr_t *hints);
EXPORT_SYMBOL x_object *x_object_find_child(x_object *starto, x_obj_attr_t *hints);
EXPORT_SYMBOL x_object *x_object_assign(x_object *o, x_obj_attr_t *attrs);
EXPORT_SYMBOL x_object *x_object_vassign(x_object *o, ...);
EXPORT_SYMBOL void x_object_lost_focus(x_object *o);
EXPORT_SYMBOL int x_object_send_down(x_object *, x_object *, x_obj_attr_t *);
EXPORT_SYMBOL void x_object_ref_put(x_object *o, void (*unref_cb)(x_object *));
EXPORT_SYMBOL x_object *x_object_ref_get(x_object *o);
EXPORT_SYMBOL int x_object_get_child_count(x_object *o);
EXPORT_SYMBOL int x_object_subscribe(x_object *xobj, x_object *listener);
EXPORT_SYMBOL int x_object_unsubscribe(x_object *xobj, x_object *listener);
EXPORT_SYMBOL void x_object_reset_subscription(x_object *xobj);
EXPORT_SYMBOL void x_object_listeners_init(x_object *xobj);
EXPORT_SYMBOL void x_object_sendmsg(x_object *to, const x_object *from, const x_object *msg);
EXPORT_SYMBOL void x_object_sendmsg_multicast(x_object *from, const x_object *msg);
EXPORT_SYMBOL x_object *x_object_open(x_object *ctx, const x_string_t name, x_obj_attr_t *hints);
EXPORT_SYMBOL uint32_t x_object_get_cr(x_object *xobj);
EXPORT_SYMBOL uint32_t x_object_set_cr(x_object *xobj, uint32_t cr_);
EXPORT_SYMBOL void x_object_destroy_tree(x_object *thiz);

#ifdef XOBJECT_EVENTS
enum
{
  EVT_NONE = 0,
  EVT_IO = 1,
  EVT_TIMER,
  EVT_PERIODIC,
  EVT_SIGNAL,
  EVT_IDLE,
  EVT_OTHER,
};
#include <ev.h>
EXPORT_SYMBOL int x_object_add_watcher(x_object *obj, void *watcher, int watchtype);
EXPORT_SYMBOL void x_object_remove_watcher(x_object *obj, void *watcher, int wtype);
EXPORT_SYMBOL x_object *x_object_get_nearest_evdomain(x_object *obj, x_obj_attr_t *hints);
#endif
/**
 * @name XOBJECTNS
 * Xobject allocation API
 *
 * @{
 */
EXPORT_SYMBOL x_object *x_object_new_ns_match(const x_string_t name, const x_string_t ns,
    x_obj_attr_t *hints, BOOL prototype_alloc);
EXPORT_SYMBOL x_object *x_object_new_ns(const x_string_t name, const x_string_t ns);
X_DEPRECATED EXPORT_SYMBOL x_object *x_object_new(const x_string_t name);
/** @} */
/**
 * @ingroup XOBJECT
 */EXPORT_SYMBOL __NOINLINE__ x_object *
x_object_default_assign_cb(x_object *o, x_obj_attr_t *attrs);

#ifdef XOBJECT_DEBUG
int xobject_testser(void);
#endif
#define X_OBJECT(x) ((x_object *)(void *)x)
#define X_ALIAS(x) ((x_alias *)(void *)x)
/*#define TREE_NODE(x) ((tree_node_t *)(void *)x)*/
#define TREE_NODE(x) (((x) != NULL) ? (tree_node_t *)(void *)&(x)->entry : NULL)
#define TNODE_TO_XOBJ(x) (((x) != NULL) ? \
    (x_object *)container_of((x),(x_object *)0,entry) \
    : (x_object *)NULL)

/*************************/
/** PSEUDO VM COMMANDS ***/
/*************************/
/** Get object attribute */
#if 1
#define _AGET(x,y) (x_object_getattr(x,_XS(y)))
#define _A(x,y) (x_object_getattr(x,y))
#define _ASET(x,y,z) (x_object_setattr(x,y,z))
/** Get object child */
#define _CHLD(x,y) (x_object_get_child(x,y))
#define _C(x,y) (x_object_get_child(x,y))
/** Get object parent */
#define _PARNT(x) (x_object_get_parent(x))
#define _P(x) (x_object_get_parent(x))
#define _NXT(x) (x_object_get_next(x))
#define _SBL(x) (x_object_get_sibling(x))
#define _CPY(x) (x_object_copy(x))
#define _GNEW(x,y) (x_object_new_ns(x,y))
#define _DEL(x) (x_object_destroy(x))
#define _DEL_TREE(x) (x_object_destroy_tree(x))
#define _RMOV(x) (x_object_remove_from_parent(x))
/*#define _EQO(x,y) (x_object_match(x,y))*/
#define _TOCH(x) (x_object_touch(x))
#define _SETNM(x,y) (x_object_set_name(x,y))
#define _GETNM(x) (x_object_get_name(x))
#define _INS(x,y) (x_object_append_child(x,y))
#define _SBSCRB(x,y) (x_object_subscribe(x,y))
#define _USBSCRB(x,y) (x_object_unsubscribe(x,y))
#define _ENV(x,y) (x_object_getenv(x,y))
#define _MCAST(x,y) (x_object_sendmsg_multicast(x,y))
#define _SNDMSG(x,y,z) (x_object_sendmsg(x,y,z))
#define _ASGN(x,y) (x_object_assign(x,y))
#define _WRITE(obj,buf,len,atr) (x_object_try_write(obj,buf,len,atr))
#define _WRITEV(obj,iov,iovcnt,atr) (x_object_try_writev(obj,iov,iovcnt,atr))
#define _WRITEV2(obj,slot,iov,iovcnt,atr) (x_object_try_writev2(obj,slot,iov,iovcnt,atr))
#define _FIND(obj,x) (x_object_find(obj, x))
#define _REFGET(obj) (x_object_ref_get(obj))
#define _REFPUT(obj,cb) (x_object_ref_put(obj,cb))
#define _CRGET(obj) (x_object_get_cr(obj))
#define _CRSET(obj,cr) (x_object_set_cr(obj,cr))
#else
static  __inline x_string_t _AGET(x,y) { return (x_object_getattr(x,_XS(y))); }
static  __inline void _ASET(x,y,z) { return (x_object_setattr(x,y,z)); }
/** Get object child */
#define _CHLD(x,y) (x_object_get_child(x,y))
#define _C(x,y) (x_object_get_child(x,y))
/** Get object parent */
#define _PARNT(x) (x_object_get_parent(x))
#define _P(x) (x_object_get_parent(x))
#define _NXT(x) (x_object_get_next(x))
#define _SBL(x) (x_object_get_sibling(x))
#define _CPY(x) (x_object_copy(x))
#define _NEW(x,y) (x_object_new_ns(x,y))
#define _DEL(x) (x_object_destroy(x))
#define _DEL_TREE(x) (x_object_destroy_tree(x))
#define _RMOV(x) (x_object_remove_from_parent(x))
/*#define _EQO(x,y) (x_object_match(x,y))*/
#define _TOCH(x) (x_object_touch(x))
#define _SETNM(x,y) (x_object_set_name(x,y))
#define _GETNM(x) (x_object_get_name(x))
#define _INS(x,y) (x_object_append_child(x,y))
#define _SBSCRB(x,y) (x_object_subscribe(x,y))
#define _USBSCRB(x,y) (x_object_unsubscribe(x,y))
#define _ENV(x,y) (x_object_getenv(x,y))
#define _MCAST(x,y) (x_object_sendmsg_multicast(x,y))
#define _SNDMSG(x,y,z) (x_object_sendmsg(x,y,z))
#define _ASGN(x,y) (x_object_assign(x,y))
#define _WRITE(obj,buf,len,atr) (x_object_try_write(obj,buf,len,atr))
#define _WRITEV(obj,iov,iovcnt,atr) (x_object_try_writev(obj,iov,iovcnt,atr))
#define _FIND(obj,x) (x_object_find(obj, x))
#define _REFGET(obj) (x_object_ref_get(obj))
#define _REFPUT(obj,cb) (x_object_ref_put(obj,cb))
#define _CRGET(obj) (x_object_get_cr(obj))
#define _CRSET(obj,cr) (x_object_set_cr(obj,cr))
#endif
/** @} */

/**
 * @name XOBJECTFACTORY
 * Object Factory API
 *
 * @{
 */

#define DFL_NAMESPACE_NAME "defaultns"

struct xobject_factory
{
  struct xobject *(*x_o_new)(struct xobject_factory *f, const char *name);
  int (*x_o_release)(struct xobject *xobj);
};

/* notifications and object events specific func */
typedef struct x_path_listener
{
  struct list_entry entry;
  void  (*on_x_path_event)(void *obj);
} x_path_listener_t;

X_DEPRECATED EXPORT_SYMBOL int
x_class_register(const char *, x_objectclass *oclass);
X_DEPRECATED EXPORT_SYMBOL void
x_class_unregister(x_objectclass *oclass);

EXPORT_SYMBOL int x_class_register_ns(const char *, x_objectclass *oclass, const char *ns);
EXPORT_SYMBOL void x_class_unregister_ns(x_objectclass *oclass, const char *ns);
EXPORT_SYMBOL x_objectclass *x_obj_get_class(const char *name, const char *ns, x_obj_attr_t *attrs,
    BOOL proto_on_fail);

EXPORT_SYMBOL void x_path_listeners_notify(const char *path, x_object *xobj);
EXPORT_SYMBOL int x_path_listener_add(const char *classname, x_path_listener_t *listener);
EXPORT_SYMBOL void x_path_listener_remove(const char *classname, x_path_listener_t *oclass);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* X_OBJ_H_ */
