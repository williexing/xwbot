/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XSESSIONS_ON
#define DEBUG_PRFX "[sessions] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>
#include "sessions.h"

#if 0 // FIXME delete me
#if defined(ANDROID)
#define SESSION_WORKING_DIRECTORY "/data/data/com.xw"
#else
#define SESSION_WORKING_DIRECTORY "/tmp"
#endif
#endif

/*
  This object generates messages:

    "session-activate",
    "session-destroy",
    "candidate-new",
    //"channel-ready"

  */

/*********************************/
/*********************************/
/*    PRIVATE METHODS   */
/*********************************/

/* matching function */
static BOOL
session_equals(x_object *o, x_obj_attr_t *attrs)
{
   const char *sida, *sido;
  ENTER;

  if (!attrs)
    return TRUE;

  sida = getattr("sid", attrs);
  sido = _AGET(o, "sid");

  if ((sida && sido && NEQ(sida,sido)) || ((sida || sido) && !(sido && sida)))
    {
      TRACE("EXIT: UGLY !!!!!!!! '%s'!='%s'\n", sido, sida);
      return FALSE;
    }

  sida = getattr("from", attrs);
  sido = _AGET(o, "from");

  if ((sida && sido && NEQ(sida,sido)) || ((sida || sido) && !(sido && sida)))
    {
      TRACE("EXIT: UGLY !!!!!!!! '%s'!='%s'\n", sido, sida);
      return FALSE;
    }

  EXIT;
  return TRUE;
}

static void
__idle_task  (xevent_dom_t *loop, struct ev_idle *w, int revents)
{
//    sched_yield();
    usleep(100);
}

static void *
__session_worker(void *thiz_)
{
    struct ev_idle idle_watcher;
    x_object *thiz = (x_object *)thiz_;
  ENTER;

  TRACE("Started new thread\n");

  _REFGET(thiz);

  ev_idle_init (&idle_watcher, &__idle_task);
  ev_idle_start (thiz->loop, &idle_watcher);

#ifndef __DISABLE_MULTITHREAD__
  if (thiz->loop)
  {
      do
      {
//          ev_loop(thiz->loop, 0);
            ev_loop(thiz->loop, EVLOOP_NONBLOCK);
      }
      while ((thiz->cr & (1 << 1)) /* thread stop condition!!! */);
  }
#endif

  _REFPUT(thiz,NULL);

  EXIT;
  return NULL;
}



static x_object *
session_assign(x_object *thiz, x_obj_attr_t *attrs)
{
    x_object *msg;
    x_string_t statestr;
    x_common_session *xcs = (x_common_session *) thiz;

    x_object_default_assign_cb(thiz,attrs);

    if ((statestr=getattr("$state",attrs)) != NULL
            && EQ(statestr,_XS("active")))
    {
        /* start working thread */
        thiz->cr |= (1 << 1);

        //        x_thread_run(&o->looptid,__session_worker,o);
        if(x_thread_run(&thiz->looptid,__session_worker, (void *) xcs) != 0)
            thiz->cr &= ~(1 << 1); // started state

        xcs->state = SESSION_ACTIVE;
        msg = _GNEW("$message",NULL);
        _ASET(msg,_XS("subject"),_XS("session-activate"));
        _ASET(msg, _XS("sid"), _AGET(thiz,_XS("sid")));
        _MCAST(thiz, msg);
        _REFPUT(msg, NULL);
    }

    return thiz;
}

static int
session_tx(x_object *to, x_object *o, x_obj_attr_t *ctx_attrs)
{
  int err = 0;
  x_string_t subj;
  x_common_session *xcs = (x_common_session *) to;
  ENTER;

  // route subtree messagess
  subj = _AGET(o, "subject");
  TRACE("---------------->>>>> '%s'\n", subj);

  /**
    * just ignore messages on inactive sessions
    */
  if (xcs->state != SESSION_ACTIVE)
  {
    return -1;
  }

  if (EQ(_XS("candidate-new"),subj))
    {
      _MCAST(to, o);
      goto _endsession_;
    }

#if 0
  if (EQ(_XS("channel-ready"),subj) && xcs->state == SESSION_NEW)
    {
      _ASET(o, _XS("sid"), _AGET(to,"sid"));
      TRACE("_MCAST: ------------>>>>> '%s', %p next listener = %p(%s)\n",
          subj, to, to->listeners.next->key, (to->listeners.next) ? _GETNM(to->listeners.next->key) : "(nil)");
      _MCAST(to, o);
    }
  else
#endif
    {
      _MCAST(to, o);
    }

_endsession_:
  /* drop message */
  _REFPUT(o, NULL);

  EXIT;
  return err;
}

static void  session_remove(x_object *o)
{
    x_object *msg;
//    x_common_session *xcs = (x_common_session *) o;
    printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);

    if(o->loop)
    {
        ev_unloop(o->loop, EVUNLOOP_ALL);
        o->cr &= ~((uint32_t) (1 << 1));
        pthread_join(o->looptid,NULL);
    }

    msg = _GNEW("$message",NULL);
    _ASET(msg,_XS("subject"),_XS("session-destroy"));
    _ASET(msg, _XS("sid"), _AGET(o,_XS("sid")));
    _MCAST(o, msg);
    _REFPUT(msg, NULL);
}

static void
session_release(x_object *o)
{
//    x_object *msg;
////    x_common_session *xcs = (x_common_session *) o;
//    printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
//    msg = _GNEW("$message",NULL);
//    _ASET(msg,_XS("subject"),_XS("session-destroy"));
//    _ASET(msg, _XS("sid"), _AGET(o,_XS("sid")));
//    _MCAST(o, msg);
//    _REFPUT(msg, NULL);
    ev_loop_destroy(o->loop);
    o->loop = NULL;
}

static int
session_append(x_object *thiz, x_object *parent)
{
    ENTER;

    thiz->loop = ev_loop_new(EVFLAG_AUTO);
//        thiz->loop = ev_default_loop(EVFLAG_AUTO);

    EXIT;
    return 0;
}

static struct xobjectclass __isession_objclass;

__x_plugin_visibility
__plugin_init void
__isession_init(void)
{
  ENTER;
  __isession_objclass.cname = X_STRING("__isession");
  __isession_objclass.psize = (unsigned int) (sizeof(struct xcommonsession)
      - sizeof(x_object));
  __isession_objclass.match = &session_equals;
  __isession_objclass.tx = &session_tx;
  __isession_objclass.on_release = &session_release;
//  __isession_objclass.on_remove = &session_remove;
  __isession_objclass.on_tree_destroy = &session_remove;
  __isession_objclass.on_assign = &session_assign;
  __isession_objclass.on_append = &session_append;
  x_class_register_ns(__isession_objclass.cname, &__isession_objclass,
      "jingle");
  EXIT;
  return;
}
PLUGIN_INIT(__isession_init);
