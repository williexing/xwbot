/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <xwlib/x_config.h>
#if TRACE_XSESSIONS_ON
#define DEBUG_PRFX "[sessions] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>
#include "sessions.h"

#if defined(ANDROID)
#define SESSION_WORKING_DIRECTORY "/data/data/com.xw"
#else
#define SESSION_WORKING_DIRECTORY "/tmp"
#endif

#define CHANNEL_STATE_TRANSPORT_READY 0x1
#define CHANNEL_STATE_MEDIA_READY 0x2
#define CHANNEL_STATE_READY 0x3

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

  /*  if (EQ(_XS("channel-new"),subj))
   {
   _MCAST(to,o);
   }
   else */
  if (EQ(_XS("candidate-new"),subj))
    {
      _MCAST(to, o);
    }
  else if (EQ(_XS("channel-ready"),subj) && xcs->state == SESSION_NEW)
    {
      _ASET(o, _XS("sid"), _AGET(to,"sid"));
      TRACE("_MCAST: ------------>>>>> '%s', %p next listener = %p(%s)\n",
          subj, to, to->listeners.next->key,
          (to->listeners.next) ? _GETNM(to->listeners.next->key) : "(nil)");
      _MCAST(to, o);
    }

  /* drop message */
  _REFPUT(o, NULL);

  EXIT;
  return err;
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
  x_class_register_ns(__isession_objclass.cname, &__isession_objclass,
      "jingle");
  EXIT;
  return;
}
PLUGIN_INIT(__isession_init);

/* matching function */
static BOOL
iostream_equals(x_object *chan, x_obj_attr_t *attrs)
{
  const char *nama, *namo;
  ENTER;

  if (!attrs)
    return FALSE;

  nama = getattr("name", attrs);
  namo = _AGET(chan, "name");

  if ((nama && namo && NEQ(nama,namo)) || ((nama || namo) && !(nama && namo)))
    {
      TRACE("EXIT Not EQUAL '%s'!='%s'\n", nama, namo);
      return FALSE;
    }

  return TRUE;
}

/**
 * @todo Possibly when passing messages through \
multiple objects each of them should add their own \
attributes in plain mode and not hierarchical ??! \
This will allow fast detection of message recipient objects.
 *
 */
static int
iostream_tx(x_object *chan, x_object *msg, x_obj_attr_t *ctx_attrs)
{
  int err;
  ENTER;
  /* act as message relay: set channel fields and pass
   * message downstrem
   */
  _ASET(msg, "channel-name", _GETNM(chan));
  err = x_object_send_down(_PARNT(chan), msg, NULL);

  EXIT;

  return err;
}

static void
__iostream_on_child_append(x_object *chan, x_object *child)
{
  x_object *msg;
  x_object *transport;
  x_object *media;
  x_string_t _chnam_ = _GETNM(child);
  x_io_stream *stream_ = (x_io_stream *) chan;
  x_obj_attr_t hints =
    { 0, 0, 0, };

  TRACE("-->> New child object appended '%s' <<--\n", _GETNM(child));
#if 0
  if ((stream_->regs.state & 0x3) != CHANNEL_STATE_READY)
    {
      transport = _CHLD(chan,_XS("transport"));
      if (transport)
      stream_->regs.state |= CHANNEL_STATE_TRANSPORT_READY;

      media = _CHLD(chan,_XS("media"));
      if (media)
      stream_->regs.state |= CHANNEL_STATE_MEDIA_READY;

      if (media && transport)
        {
          msg = _NEW(_XS("$message"),NULL);
          _ASET(msg, _XS("subject"), _XS("channel-ready"));
          _ASET(msg, _XS("chname"), _GETNM(chan));
          x_object_send_down(_PARNT(chan), msg, &hints);
        }
    }
#else

  if ((stream_->regs.state & 0x3) == CHANNEL_STATE_READY)
    return;

  else if (_chnam_)
    {
      if (EQ(_chnam_,_XS("transport")))
        {
          stream_->regs.state |= CHANNEL_STATE_TRANSPORT_READY;
        }
      else if (EQ(_chnam_,MEDIA_IN_CLASS_STR))
        {
          stream_->regs.state |= CHANNEL_STATE_MEDIA_READY;
        }
      else if (EQ(_chnam_,MEDIA_OUT_CLASS_STR))
        {
          stream_->regs.state |= CHANNEL_STATE_MEDIA_READY;
        }
    }

  if ((stream_->regs.state & 0x3) == CHANNEL_STATE_READY)
    {
      msg = _NEW(_XS("$message"),NULL);
      _ASET(msg, _XS("subject"), _XS("channel-ready"));
      _ASET(msg, _XS("chname"), _GETNM(chan));
      TRACE ("CHANNEL READY '%s'\n",_GETNM(chan));
      x_object_send_down(_PARNT(chan), msg, &hints);
    }
#endif
}

static x_object *
iostream_on_assign(x_object *chan, x_obj_attr_t *attrs)
{
  x_object *tmpo;
  const char *attr;

  ENTER;

  x_object_default_assign_cb(chan, attrs);

  if ((attr = getattr("mtype", attrs)))
    {
      if (EQ(attr,_XS("video")))
        {
          tmpo = _NEW(_XS("video_player"),_XS("gobee:media"));
          _SETNM(tmpo, _XS("dataplayer"));
          _INS(chan, tmpo);

          tmpo = _NEW(_XS("camera"),_XS("gobee:media"));
          _SETNM(tmpo, _XS("datasrc"));
          _INS(chan, tmpo);
        }
      else
        if (EQ(attr,_XS("audio")))
        {
            tmpo = _NEW(_XS("audio_player"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("dataplayer"));
            _INS(chan, tmpo);

            tmpo = _NEW(_XS("microphone"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("datasrc"));
            _INS(chan, tmpo);
        }
    }

  EXIT;
  return chan;
}

static
void
iostream_on_create(x_object *o)
{
  x_io_stream *stream_ = (x_io_stream *) o;
  stream_->regs.state = 0;
}

static struct xobjectclass __iostream_objclass;
__x_plugin_visibility
__plugin_init void
__iostream_init(void)
{
  ENTER;

  __iostream_objclass.cname = X_STRING("__iostream");
  __iostream_objclass.psize = (unsigned int) (sizeof(x_io_stream)
      - sizeof(x_object));
  __iostream_objclass.on_create = &iostream_on_create;
  __iostream_objclass.match = &iostream_equals;
  __iostream_objclass.on_child_append = &__iostream_on_child_append;
  __iostream_objclass.tx = &iostream_tx;
  __iostream_objclass.on_assign = &iostream_on_assign;

  x_class_register_ns(__iostream_objclass.cname, &__iostream_objclass,
      "jingle");

  EXIT;
  return;
}
PLUGIN_INIT(__iostream_init);

