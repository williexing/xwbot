/*
 * cmdapi.cpp
 *
 *  Created on: Nov 4, 2011
 *      Author: artemka
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[xcmdapi++] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include <plugins/sessions_api/sessions.h>

#include "x_cmdapi.h"

const char *xcmdapi::cmdName = "mplayer";
const char *xcmdapi::cmdXmlns = "http://jabber.org/protocol/commands";

int
xcmdapi::equals(x_obj_attr_t *_o)
{
  const char *xmlns;
  ENTER;

  xmlns = getattr((KEY) "xmlns", _o);

  if ((x_strncasecmp(xmlns, xcmdapi::cmdXmlns, strlen(xcmdapi::cmdXmlns)) == 0))
    return (int) true;
  else
    return (int) false;
}

void
xcmdapi::on_create()
{
  this->__setattr("xmlns", xcmdapi::cmdXmlns);
  this->__setattr("node", xcmdapi::cmdName);
}

void
xcmdapi::on_remove()
{
  TRACE("%s\n", this->__get_name());
}

void
xcmdapi::on_release()
{
  ENTER;
  EXIT;
  // assert
  BUG();
}

int
xcmdapi::on_append(__x_object *parent)
{
  TRACE("%s parent(%p)\n", this->__get_name(), (void *)parent);
  return 0;
}

int
xcmdapi::classmatch(x_obj_attr_t *attrs)
{
  const char *node;
  ENTER;
  if (attrs)
    {
      node = getattr((KEY) "node", attrs);
      if (node && EQ(node,xcmdapi::cmdName))
        {
          TRACE("OK. %s\n", node);
          return (int) true;
        }
    }
  EXIT;
  return (int) false;
}

xcmdapi::ACTION
xcmdapi::get_action()
{
  const char *action = (*this)["action"];
  if (!action)
    return xcmdapi::NOACTION;

  if (EQ(action,"execute"))
    {
      return xcmdapi::EXECUTE;
    }
  else if (EQ(action,"cancel"))
    {
      return xcmdapi::CANCEL;
    }
  else if (EQ(action,"complete"))
    {
      return xcmdapi::COMPLETE;
    }
  else if (EQ(action,"prev"))
    {
      return xcmdapi::PREV;
    }
  else if (EQ(action,"next"))
    {
      return xcmdapi::NEXT;
    }

  return xcmdapi::NOACTION;
}

/**
 * Make jingle call to requester
 */
void
xcmdapi::CallBack(const char *jid)
{
  gobee::__x_object *_chan_;
  gobee::__x_object *_sess_ = NULL;
  x_obj_attr_t hints =
    { NULL, NULL, NULL, };

  const char *sid;

  x_object_print_path(this->xobj.bus, 0);
  x_object_print_path(X_OBJECT(this), 0);

  BUG_ON(!jid);

  if (jid)
    {
      //      sid = "MplayerSess12345";
      //      setattr("sid", (VAL) sid, &hints);
      _sess_ = static_cast<gobee::__x_object *>((void *) x_session_open(jid,
          X_OBJECT(this), &hints, X_CREAT));
      //      attr_list_clear(&hints);
    }

  if (!_sess_)
    {
      BUG();
    }

  /* open/create audio channel */
  _chan_ = static_cast<gobee::__x_object *>((void *) x_session_channel_open2(
      X_OBJECT(_sess_), "m.A"));
  //  setattr("pwd", "MplayerAudio_pwd", &hints);
  //  setattr("ufrag", "todo", &hints);
  setattr("mtype", "audio", &hints);
  _ASGN(X_OBJECT(_chan_), &hints);
  attr_list_clear(&hints);

  x_session_channel_set_transport_profile(X_OBJECT(_chan_), _XS("__icectl"),
      &hints);
  x_session_channel_set_media_profile(X_OBJECT(_chan_), _XS("__rtppldctl"));

  setattr("clockrate", "16000", &hints);
  x_session_add_payload_to_channel(X_OBJECT(_chan_), _XS("110"), _XS("SPEEX"),
      &hints);

#if 0
  /* open/create video channel */
  _chan_ = static_cast<gobee::__x_object *> ((void *) x_session_channel_open2(
          X_OBJECT(_sess_), "m.V"));

  //  setattr("pwd", "MplayerVideo_pwd", &hints);
  //  setattr("ufrag", "todo", &hints);
  x_session_channel_set_transport_profile(X_OBJECT(_chan_), _XS("__icectl"),
      &hints);
  x_session_channel_set_media_profile(X_OBJECT(_chan_), _XS("__rtppldctl"));
#endif
  //  attr_list_clear(&hints);

}

gobee::__x_object *
xcmdapi::onExecute(gobee::__x_object *req)
{
  gobee::__x_object *res = new ("jabber:x:data", "x") gobee::__x_object();
  gobee::__x_object *title = new ("jabber:x:data", "title") gobee::__x_object();
  gobee::__x_object *instructions =
      new ("jabber:x:data", "instructions") gobee::__x_object();
  gobee::__x_object *field = new ("jabber:x:data", "field") gobee::__x_object();

  gobee::__x_object *form = req->get_child("x");

  // construct first response
  res->add_child(title);
  res->add_child(instructions);
  res->add_child(field);

  title->write_content("Gobee Media Player v1.0",
      strlen("Gobee Media Player v1.0"));

  instructions->write_content("Fill form correctly",
      strlen("Fill form correctly"));

  field->write_content("<value>3</value> <value>5</value>"
      "<option label='Single-User'><value>1</value></option>"
      "<option label='Non-Networked Multi-User'><value>2</value></option>"
      "<option label='Full Multi-User'><value>3</value></option>"
      "<option label='X-Window'><value>5</value></option>",
      strlen("<value>3</value> <value>5</value>"
          "<option label='Single-User'><value>1</value></option>"
          "<option label='Non-Networked Multi-User'><value>2</value></option>"
          "<option label='Full Multi-User'><value>3</value></option>"
          "<option label='X-Window'><value>5</value></option>"));

  field->__setattr("var", "state");
  field->__setattr("label", "Player State");
  field->__setattr("type", "list-multi");

  res->__setattr("xmlns", "jabber:x:data");
  res->__setattr("type", "form");

  this->CallBack(req->__getenv("from"));

  return res;
}

gobee::__x_object *
xcmdapi::onNext(gobee::__x_object *req)
{
  gobee::__x_object *form = NULL;
  form = req->get_child("x");
  return NULL;
}

gobee::__x_object *
xcmdapi::get_response(gobee::__x_object *req, xcmdapi::ACTION action)
{
  gobee::__x_object *resp =
      new (DFL_NAMESPACE_NAME, "protoobject") gobee::__x_object();
  gobee::__x_object *x = NULL;

  switch (action)
    {
  case xcmdapi::COMPLETE:
    break;
  case xcmdapi::CANCEL:
    break;
  case xcmdapi::PREV:
    break;
  case xcmdapi::NEXT:
    x = this->onNext(req);
    break;
  case xcmdapi::NOACTION:
  case xcmdapi::EXECUTE:
  default:
    x = this->onExecute(req);
    break;
    };

  resp->__set_name("command");
  resp->__setattr("node", (*this)["node"]);
  resp->__setattr("xmlns", (*this)["xmlns"]);
  resp->__setattr("status", "executing");
  resp->__setattr("sessionid", "55667788");

  if (x)
    resp->add_child(x);

  x_object_print_path(X_OBJECT(resp), 0);

  return resp;
}

gobee::__x_object *
xcmdapi::operator+=(x_obj_attr_t *attr)
{
  const char *str;
  ENTER;
  x_object_default_assign_cb(X_OBJECT(this), attr);
  str = this->__getenv("id");
  TRACE("%s\n", str);
  this->__setattr("#id", str);
  str = this->__getenv("type");
  TRACE("%s\n", str);
  this->__setattr("#type", str);
  EXIT;
  return (gobee::__x_object *) (void *) this;
}

// demultiplexer
void
xcmdapi::finalize()
{
  x_obj_attr_t *hints;
  gobee::__x_object *obj2;
  gobee::__x_object *busobj;
  const char *node;
  const char *id;
  ENTER;

  busobj = this->get_bus();
  node = (*this)["node"];
  id = this->__getattr("#id");

  if (node)
    {
      obj2 = busobj->get_object_at_path("__proc/commands/");
      if (obj2)
        obj2 = obj2->get_child(node);

      hints = new x_obj_attr_t;
      attr_list_init(hints);

      setattr((KEY) "#type", (VAL) "result", hints);
      setattr((KEY) "#id", (VAL) id, hints);

      this->tx(((xcmdapi *) obj2)->get_response(this, this->get_action()),
          hints);
    }

  EXIT;
}

static void
___on_x_path_event(gobee::__x_object *obj)
{
  x_object *tmp;
  gobee::__x_object *_o =
      new ("http://jabber.org/protocol/commands", "command") xcmdapi();
  ENTER;

  _o->__set_name("mplayer");
  _o->__setattr("name", "Media Player Controller");
  //  _o->__setattr("jid", obj->__getenv("jid"));

  tmp = x_object_add_path(X_OBJECT(obj), "__proc/commands", NULL);
  x_object_append_child(tmp, X_OBJECT(_o));

  EXIT;
}

static xcmdapi oproto;
static gobee::__path_listener cmd_bus_listener;

#if !defined (__clang__)
__x_plugin_visibility
__plugin_init 
#else
static
#endif
void
mplayer_plugin_init(void)
{
  gobee::__x_class_register_xx(&oproto, sizeof(xcmdapi),
      "http://jabber.org/protocol/commands", "command");

  cmd_bus_listener.lstnr.on_x_path_event = (void
  (*)(void*)) &___on_x_path_event;x_path_listener_add
  ("/", (x_path_listener_t *) (void *) &cmd_bus_listener);

  return;
}

#if defined (__clang__)
class static_initializer {
    void (*_fdtor)(void);
    
public:
    static_initializer(void (*fctor)(void), void (*fdtor)(void)): _fdtor(fdtor){
        if(fctor) fctor();
    }
    ~static_initializer() {}
};

static static_initializer __minit(&mplayer_plugin_init,NULL);
#endif

