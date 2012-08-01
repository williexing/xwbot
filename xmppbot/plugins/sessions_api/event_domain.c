/**
 * payload_ctl.c
 *
 *  Created on: Jan 12, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[__payloadctl] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

/**
 *
 */
typedef struct _evt_domain
{
  x_object xobj;
  struct
  {
    int state;
    u_int32_t cr0;
  } regs;
  struct ev_loop *eventloop;

#ifndef __DISABLE_MULTITHREAD__
  /** thread id */
  THREAD_T loop_tid;
#endif

} x_evt_domain;

static void *
__evtdom_icectl_worker(void *ctx)
{
  int err;
  x_evt_domain *evtd = (x_evt_domain *) (void *) ctx;

  ENTER;
  do
    {
      ev_run(evtd->eventloop, 0);
    }
  while (!(evtd->regs.cr0 & (1 << 0)) /* thread stop condition!!! */);

  EXIT;
  return NULL;
}

static void
evtdom_on_create(x_object *o)
{
  ENTER;
  EXIT;
}

static int UNUSED
evtdom_on_append(x_object *o, x_object *parent)
{
  int err;
  x_evt_domain *evtd = (x_evt_domain *) (void *) o;
  ENTER;

#ifndef __DISABLE_MULTITHREAD__
#ifndef WIN32
  err = pthread_create(&evtd->loop_tid, NULL,
      &__evtdom_icectl_worker, evtd);
#endif
#else
  TRACE("Unimplemented FUNCTION!!!!!!!!!!!\n");
#endif

  EXIT;
  return err;
}

static void UNUSED
evtdom_remove_cb(x_object *o)
{
  ENTER;
  EXIT;
}

static void UNUSED
evtdom_on_child_append(x_object *o, x_object *child)
{
  ENTER;
  TRACE("Appended '%s'\n",x_object_get_name(child));
  EXIT;
}

static void UNUSED
evtdom_release_cb(x_object *o)
{
  ENTER;
  EXIT;
}

/* matching function */
static BOOL
evtdom_equals(x_object *o, x_obj_attr_t *attrs)
{
  return TRUE;
}


static struct xobjectclass  __evtdomain_objclass;
__x_plugin_visibility __plugin_init void __evtdomain_init(void)
{
	ENTER;
	__evtdomain_objclass.cname = X_STRING("__evtdomain");
	__evtdomain_objclass.psize = (unsigned int)(sizeof(x_object)
		- sizeof(x_object));

	__evtdomain_objclass.match = &evtdom_equals;
	__evtdomain_objclass.on_create = &evtdom_on_create;
	__evtdomain_objclass.on_append = &evtdom_on_append;
    __evtdomain_objclass.on_child_append = &evtdom_on_child_append;
	__evtdomain_objclass.on_remove = &evtdom_remove_cb;
	__evtdomain_objclass.on_release = &evtdom_release_cb;

	x_class_register(__evtdomain_objclass.cname, &__evtdomain_objclass);
	EXIT;
	return;
}

PLUGIN_INIT(__evtdomain_init);
