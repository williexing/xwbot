/*
 * x_objfactory.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#include <stdio.h>

#undef DEBUG_PRFX

#include "x_config.h"
#if TRACE_XOFACTORY_ON
#define DEBUG_PRFX "[objfactory] "
#endif

#include "x_utils.h"
#include "x_obj.h"

/** default namespace map table
 * @todo Complete namespace support
 */
static struct ht_cell *nsmap[HTABLESIZE];

typedef struct x_path
{
  list_entry_t *entry;
  char *name;
} x_path_t;

typedef struct x_ns
{
  const char *nsname;
  struct ht_cell classmap[HTABLESIZE];
} x_ns_t;

#if defined(__STRICT_ANSI__) || defined(WIN32)
static struct xobjectclass __protoclass =
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0,0,0,0,
#if 0
    0 /*on_subtree_event*/,
    0 /*on_instance_event*/,
#endif
      { 0, 0} /*lentry*/, "__protoclass", 0, 0};
#else
static struct xobjectclass __protoclass =
  { .cname = "__protoclass", .ns = "gobee", };
#endif

static int initialized = 0;
CS_DECL(factory_CS);

#define CHK_STATE {if(!initialized){ \
    CS_INIT(CS2CS(factory_CS)); \
    initialized++; \
}}

#if 0
#ifdef WIN32
__plugin_ptr const plugin_t InitSegStart = (__t_func__)0;
__plugin_ptr const plugin_t InitSegEnd = (__t_func__)0;
#endif

void
x_obj_factory_init(void)
  {
    plugin_t *fu = (plugin_t *)&InitSegStart;
    plugin_t *fu__ = (plugin_t *)&InitSegEnd;

    for (; fu < (plugin_t *)&InitSegEnd; ++fu)
      {
        if (*fu)
          {
            (*fu)();
          }
      }
  }
#endif

static x_ns_t *
x_class_get_ns(const char *nsname)
{
  int i;
  x_ns_t *_ns = NULL;

  _ns = (x_ns_t *) ht_lowcase_get((KEY) nsname, (struct ht_cell **) &nsmap, &i);

  if (!_ns)
    {
      _ns = x_malloc(sizeof(x_ns_t));
      x_memset(_ns,0,sizeof(x_ns_t));
      ht_set((KEY) nsname, (VAL) _ns, (struct ht_cell **) &nsmap);
    }

  return _ns;
}

x_objectclass *
x_obj_get_class(const char *name, const char *ns, x_obj_attr_t *attrs,
    BOOL proto_on_fail)
{
  int i;
  list_entry_t *_r;
  x_objectclass *ret_;
  x_objectclass *ret;
  x_ns_t *namespace;
  char *_ns = (char *) ns;
  ENTER;

  if (!_ns)
    _ns = DFL_NAMESPACE_NAME;

  CHK_STATE;
  CS_LOCK(CS2CS(factory_CS));

  namespace = x_class_get_ns(_ns);

  ret = (x_objectclass *) ht_lowcase_get((KEY) name, (struct ht_cell **)
      &namespace->classmap, &i);

  _r = ret ? &ret->lentry : NULL;

  for (; _r; _r = _r->next)
    {
      ENTER;
      ret_ = (x_objectclass *) container_of(_r,(x_objectclass *)0,lentry);
      if (ret_->classmatch && (ret_->classmatch(NULL, attrs) > 0))
        {
          ret = ret_;
          break;
        }
      if (_r == &ret->lentry)
        break;
    }

  CS_UNLOCK(CS2CS(factory_CS));

  EXIT;

  if (ret)
    return ret;

  if (proto_on_fail)
    return &__protoclass;

  return NULL;
}

/**
 * Registers new object class
 * @deprecated Replaced with x_class_register_ns()
 */
int
x_class_register(const char *alias, x_objectclass *oclass)
{
  return x_class_register_ns(alias, oclass, NULL);
}

/**
 * Registers new object class
 * @deprecated Replaced with x_class_unregister_ns()
 */
void
x_class_unregister(x_objectclass *oclass)
{
  x_class_unregister_ns(oclass, NULL);
}

/**
 * Registers new object class in a namespace
 *
 * @param alias One of class names
 * @param oclass Pointer to objectclass descriptor
 * @param ns Namespace of alias
 */
int
x_class_register_ns(const char *alias, x_objectclass *oclass, const char *ns)
{
  char *_ns = (char *) ns;
  int i;
  x_objectclass *myclass;
  x_ns_t *namespace;

  if (!_ns)
    _ns = DFL_NAMESPACE_NAME;

  TRACE(">> Registering object class '%s' as '%s' in '%s'\n",
      oclass->cname, alias, _ns);

  CHK_STATE;

  CS_LOCK(CS2CS(factory_CS));

  xw_list_init(&oclass->lentry);

  namespace = x_class_get_ns(_ns);

  myclass = (x_objectclass *) ht_get((KEY) alias,
      (struct ht_cell **) &namespace->classmap, &i);

  if (!myclass)
    {
      ht_set((KEY) alias, (VAL) oclass,
          (struct ht_cell **) &namespace->classmap);
    }
  else
    {
      xw_list_insert(&myclass->lentry, &oclass->lentry);
    }

  TRACE(".\n");
  CS_UNLOCK(CS2CS(factory_CS));

  TRACE(".\n");
  return 0;
}

/**
 * Registers new object class
 * @deprecated Replaced with x_class_unregister_ns()
 */
void
x_class_unregister_ns(x_objectclass *oclass, const char *ns)
{
  x_ns_t *namespace;
  char *_ns = (char *) ns;
  if (!_ns)
    _ns = DFL_NAMESPACE_NAME;
  TRACE("Removing object class '%s' from '%s'\n",oclass->cname,_ns);

  CHK_STATE;
  CS_LOCK(CS2CS(factory_CS));

  namespace = x_class_get_ns(_ns);
  ht_del((KEY) oclass->cname, (struct ht_cell **) &namespace->classmap);

  CS_UNLOCK(CS2CS(factory_CS));

  return;
}

/**
 * This is used when somebody wants
 * to get notified when new path created
 * and initialized.
 */
static struct ht_cell *x_path_listeners_tbl[HTABLESIZE];

void
x_path_listeners_notify(const char *path, x_object *xobj)
{
  int idx;
  x_path_listener_t *listeners_list;
  x_path_listener_t *lstnr;

  ENTER;

  TRACE("Notifying x_path listeners for '%s'\n",path);
  listeners_list = (x_path_listener_t *) ht_get((KEY) path,
      (struct ht_cell **) &x_path_listeners_tbl, &idx);

  if (listeners_list)
    {
      struct list_entry *le = &listeners_list->entry;
      idx = 0;
      do
        {
          lstnr
              = (x_path_listener_t *) container_of(le,(x_path_listener_t *)0,entry);

          if (lstnr->on_x_path_event)
            lstnr->on_x_path_event((void *) xobj);

          if (le->next == &listeners_list->entry)
            le = NULL;
          else
            le = le->next;
        }
      while (le);
    }
  EXIT;
}

/** Registers tree path listener */
int
x_path_listener_add(const char *path, x_path_listener_t *lstnr)
{
  int idx;
  x_path_listener_t *listeners_list;

  TRACE("Registering x_path listener for '%s'\n",path);

  CHK_STATE;
  CS_LOCK(CS2CS(factory_CS));

  xw_list_init(&lstnr->entry);
  listeners_list = (x_path_listener_t *) ht_get((KEY) path,
      (struct ht_cell **) &x_path_listeners_tbl, &idx);

  if (!listeners_list)
    {
      ht_set((KEY) path, (VAL) lstnr, (struct ht_cell **) &x_path_listeners_tbl);
    }
  else
    {
      xw_list_insert(&listeners_list->entry, &lstnr->entry);
    }

  CS_UNLOCK(CS2CS(factory_CS));

  return 0;
}

/** Unregisters path listener  */
void
x_path_listener_remove(const char *path, x_path_listener_t *lstnr)
{
  int idx;
  x_path_listener_t *listeners_list;

  TRACE("Removing x_path listener for '%s'\n",path);

  CHK_STATE;
  CS_LOCK(CS2CS(factory_CS));

  listeners_list = (x_path_listener_t *) ht_get((KEY) path,
      (struct ht_cell **) &x_path_listeners_tbl, &idx);

  if (listeners_list)
    {
      if (xw_list_is_empty(&listeners_list->entry))
        {
          if (lstnr == listeners_list)
            ht_del((KEY) path, (struct ht_cell **) &x_path_listeners_tbl);
        }
      else
        {
          struct list_entry *le = &listeners_list->entry;
          for (; le && le != &lstnr->entry; le = le->next)
            {
              /* not found, the end of list */
              if (le->next == &listeners_list->entry)
                le = NULL;
            }

          if (le)
            {
              if (le == &listeners_list->entry)
                {
                  /* if list head is removing then set to next listener node */
                  listeners_list
                      = (x_path_listener_t *) container_of(le->next,(x_path_listener_t *)0,entry);
                  ht_set((KEY) path, (VAL) listeners_list,
                      (struct ht_cell **) &x_path_listeners_tbl);
                }
              xw_list_remove(le);
            }
        }
    }

  CS_UNLOCK(CS2CS(factory_CS));

  return;
}

/**
 * This is used when somebody wants
 * to get notified when new path created
 * and initialized.
 */
static struct ht_cell *x_class_listeners_tbl[HTABLESIZE];
