/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[message] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>

#include <xmppagent.h>

#include <plugins/sessions_api/sessions.h>

enum
{
  MSG_STATE_GNEW = 0, MSG_STATE_HAS_SESSION,
};

struct msg_object
{
  x_object super;
  x_object *iosession;
  struct
  {
    int state;
    u_int32_t cr0;
  } regs;
};

/*/////////////////////////////////////////*/
/*/////////////////////////////////////////*/
/*///////////// IO FUNCTIONS //////////////*/
/*/////////////////////////////////////////*/
/*/////////////////////////////////////////*/
static int
msg_write_back_handler(x_object *cbdata, void *buf, ssize_t len)
{
  x_object *msg;
  x_object *tmp;
  struct msg_object *msgo = (struct msg_object *) cbdata;
  ENTER;

//  msg = x_object_new(msgo->super.objclass->cname);
  msg = _GNEW("$message", NULL);
  _SETNM(msg, msgo->super.objclass->cname);
  x_object_setattr(msg, "from", x_object_getenv(X_OBJECT(msgo), "jid"));
  x_object_setattr(msg, "to", x_object_getattr(X_OBJECT(msgo), "from"));
  x_object_setattr(msg, "type", "chat");
  x_object_setattr(msg, "id", "12345");
  x_object_setattr(msg, "xml:lang", "en");

  tmp = x_object_new("body");
  x_string_write(&tmp->content, buf, len);
  x_object_append_child(msg, tmp);

  x_object_send_down(msgo->super.bus, msg, NULL);
  _REFPUT(msg, NULL);
  
  EXIT;
  return 0;
}

/*/////////////////////////////////////////*/
/*/////////////////////////////////////////*/

/** matching function */
static BOOL
msg_match(x_object *o, x_obj_attr_t *attrs)
{
  const char *from1, *from2;
  ENTER;

  from1 = getattr("from", attrs);
  from2 = x_object_getattr(o, "from");

  if (((from1 && from2) && !EQ(from1,from2)) || !(from1 && from2))
    {
      TRACE("FAIL: %s != %s, EXIT\n", from1, from2);
      return FALSE;
    }

  return TRUE;
}

/* constructor */
static void
msg_on_create(x_object *o)
{
  ENTER;
  EXIT;
  return;
}

static int
msg_on_append(x_object *o, x_object *parent)
{
  ENTER;
  EXIT;
  return 0;
}

/** handler of removing event (appears when object is removing from its parent) */
static void
msg_on_remove(x_object *o)
{
  ENTER;
  EXIT;
}

/** default constructor */
static void
msg_on_release(x_object *o)
{
  ENTER;
  EXIT;
}

static int
msg_msg_recv(x_object *to, const x_object *from, const x_object *msg_)
{
  x_object *msg;
  x_object *tmp;
  struct msg_object *msgo = (struct msg_object *) to;
  ENTER;

  TRACE("New Message\n");

  if (msg_->content.cbuf)
    TRACE("RX:{\n%s\n}\n", msg_->content.cbuf);

  msg = _GNEW(_XS("$message"),NULL);
  _SETNM(msg,_XS("message"));

  _ASET(msg, "from", _ENV(X_OBJECT(msgo), "jid"));
  _ASET(msg, "to", _AGET(X_OBJECT(msgo), "from"));
  _ASET(msg, "type", "chat");
  _ASET(msg, "id", "12345");
  _ASET(msg, "xml:lang", "en");

  tmp = _GNEW(_XS("$message"),NULL);
  _SETNM(tmp,_XS("body"));

  x_object_set_content(tmp, msg_->content.cbuf, msg_->content.pos);

  /* append and don't fire callbacks */
  x_object_append_child_no_cb(msg, tmp);

//  x_object_send_down(msgo->super.bus, msg, NULL);
  x_object_send_down(_PARNT(to), X_OBJECT(msg), NULL);
  _REFPUT(msg, NULL);
  
  EXIT;
  return 0;
}

/** assignment handler */
static x_object *
msg_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  x_obj_attr_t hints =
    { NULL, NULL, NULL, };
  x_string_t from = NULL;
  x_object *_chan_;
  struct msg_object *msgo = (struct msg_object *) (void *) o;
  ENTER;

  x_object_default_assign_cb(o, attrs);
  TRACE("message from2=%s\n", x_object_getattr(o, "from"));

  if (msgo->regs.state != MSG_STATE_HAS_SESSION)
    {
      TRACE("Creating session\n");

      from = x_object_getattr(o, "from");
      BUG_ON(!from);
      if (from)
        {
          TRACE("Setting system env.\n");
          msgo->iosession = X_OBJECT(x_session_open(from, o, &hints, X_CREAT));
          msgo->regs.state = MSG_STATE_HAS_SESSION;
          attr_list_clear(&hints);

          _SBSCRB(msgo->iosession,o);

          /* set channel profiles */
          _chan_ = x_session_channel_open2(X_OBJECT(msgo->iosession), "msg");

          setattr("mtype", "message", &hints);
          _ASGN(X_OBJECT(_chan_), &hints);
          attr_list_clear(&hints);

          x_session_channel_set_transport_profile_ns(X_OBJECT(_chan_),
              _XS("__ibshell_t_profile"), _XS("gobee:media"), &hints);
          x_session_channel_set_media_profile_ns(X_OBJECT(_chan_),
              _XS("__ibshell_m_profile"), _XS("gobee:media"));

          // subsribe for tranport messages
          _SBSCRB(_CHLD(_chan_,"transport"),o);

        }
    }

  EXIT;
  return o;
}

static void
msg_exit(x_object *o)
{
  x_object *outchan;
  x_object *out;
  x_object *body;
  struct msg_object *msgo = (struct msg_object *) (void *) o;
  ENTER;

  printf("%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);

//  x_object_print_path(o, 0);

  body = x_object_get_child(o, "body");

  if (body && msgo->iosession)
    {
      if (body->content.pos)
        {
          TRACE("BODY = %s\n", body->content.cbuf);

          /* now write message out */
          outchan = x_session_channel_open2(X_OBJECT(msgo->iosession), "msg");
          if (outchan)
            {
              TRACE("\n");
              if ((out = _CHLD(outchan, _XS("transport"))))
                {
                  TRACE("%s\n",body->content.cbuf);
                  _WRITE(out, body->content.cbuf, body->content.pos, NULL);
                }
              else
                {
                  TRACE("No output found\n");
                }
            }
          else
            {
              TRACE("No channel found\n");
            }
          x_string_rew(&body->content);
        }
    }

  EXIT;
}

static struct xobjectclass message_objclass;

__x_plugin_visibility
__plugin_init void
message_init(void)
{
  ENTER;

  message_objclass.cname = "message";
  message_objclass.psize = (unsigned int) (sizeof(struct msg_object)
      - sizeof(x_object));
  message_objclass.match = &msg_match;
  message_objclass.on_create = &msg_on_create;
  message_objclass.on_append = &msg_on_append;
  message_objclass.on_remove = &msg_on_remove;
  message_objclass.on_release = &msg_on_release;
  message_objclass.on_assign = &msg_on_assign;
  message_objclass.commit = &msg_exit;
  message_objclass.try_write = &msg_write_back_handler;
  message_objclass.rx = &msg_msg_recv;

  x_class_register_ns(message_objclass.cname, &message_objclass,
      "jabber:client");
  EXIT;
  return;
}
PLUGIN_INIT(message_init)
;

/* 'body' matching function */
static BOOL
msg_body_match(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;
  TRACE("qqq\n");
  EXIT;
  return TRUE;
}

static struct xobjectclass body_objclass;

__x_plugin_visibility
__plugin_init void
body_init(void)
{
  ENTER;

  body_objclass.cname = "body";
  body_objclass.psize = 0;
  body_objclass.match = &msg_body_match;

  x_class_register_ns(body_objclass.cname, &body_objclass, "jabber:client");
  EXIT;
  return;
}
PLUGIN_INIT(body_init)
;
