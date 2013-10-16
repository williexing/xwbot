/**
 * session_api.c
 *
 *  Created on: Mar 21, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XSESSIONS_API_ON
#define DEBUG_PRFX "[sessions_api] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>
#include "sessions.h"

/***********************************/
/***********************************/
/* SESSION INTERNAL API      */
/***********************************/
/***********************************/

/**
 * @brief x_session_find
 * @param root
 * @param attrs
 * @return
 */
static x_common_session *
x_session_find(x_object *root, x_obj_attr_t *attrs)
{
  x_object *_o = NULL;
  ENTER;
  _o = _FIND(_CHLD(root,"__isession"), attrs);
  EXIT;
  return (x_common_session *) _o;
}

/**
  * FIXME This must return x_object* !!!:
  *
 * @brief x_session_open
 * @param jid_remote
 * @param context
 * @param hints
 * @param flags
 * @return
 */
x_common_session *
x_session_open(const char *jid_remote, x_object *context, x_obj_attr_t *hints,
    int flags)
{
  char sidbuf[64];
  const char *sid;
  char *sbuf;
  x_object *_o = NULL;
  x_object *_sesso = NULL;
  x_common_session *xcs = NULL;

  BUG_ON(!jid_remote);

  JID_TO_SID_A(sbuf, jid_remote);

  _sesso = _CHLD(context->bus, "__sessions");
  /**
   * @todo this entry must be created in path listener callback
   * when new bus created.
   */
  if (!_sesso)
    {
      _sesso = x_object_mkpath(context->bus, "__sessions");
      BUG_ON(!_sesso);
    }

  /*
   * get session holder for given remote peer (sbuf)
   * and create new one if not found
   */
  _o = _CHLD(_sesso, sbuf);
  if (_o)
    xcs = x_session_find(_o, hints);
  else
    {
        _o = _GNEW("$message",NULL); // empty object
      _SETNM(_o,sbuf);
      _INS(_sesso, _o);
    }

  if (!xcs && (flags == X_CREAT))
    {
      xcs = (x_common_session *) _GNEW("__isession", "jingle");
      _ASGN(X_OBJECT(xcs), hints);

      if (!(sid = getattr("sid", hints)))
        {
          x_sprintf(sidbuf, "%d", rand());
          sid = sidbuf;
        }
      _ASET((x_object *) xcs, "sid", sid);
      _ASET((x_object *) xcs, "remote", jid_remote);
      /* run session by appending to tree */
      _INS(_o, (x_object *) xcs);
    }

  /*  x_object_print_path(context->bus, 0);*/
  return xcs;
}

/**
  * Closes and releases session object
  *
 * @brief x_session_close
 * @param jid_remote
 * @param context
 * @param hints
 * @param flags
 * @return
 */
int
x_session_close(const char *jid_remote,
                x_object *context, x_obj_attr_t *hints,
                int flags)
{
  char *sbuf;
  x_object *_o = NULL;
  x_object *_sesso = NULL;
  x_common_session *xcs = NULL;

  ENTER;

  BUG_ON(!jid_remote);

  TRACE("Closing session from(%s):sid(%s)\n",
        jid_remote, getattr("sid", hints));

  JID_TO_SID_A(sbuf, jid_remote);

  _sesso = _CHLD(context->bus, "__sessions");
  if (!_sesso)
        return 0;

  /*
   * get session holder for given remote peer (sbuf)
   * and create new one if not found
   */
  _o = _CHLD(_sesso, sbuf);
  if (!_o)
      return 0;

  xcs = x_session_find(_o, hints);
  if (!xcs)
      return 0;

  /* full session subtree destroy */
  TRACE("Destroying session sid(%s)\n",getattr("sid", hints));
  x_object_destroy_tree(xcs);
  TRACE("Destroyed session ok\n");

  _RMOV(xcs);

  /*  x_object_print_path(context->bus, 0);*/
  return 0;
}

/**
 * @brief x_session_channel_open2
 * @param sesso
 * @param ch_name
 * @return
 */
x_object *
x_session_channel_open2(x_object *sesso, const char *ch_name)
{
  x_object *cho;
  cho = _CHLD(sesso, ch_name);
  if (!cho)
    {
      cho = _GNEW(_XS("__iostream"),_XS("jingle"));
      _SETNM(cho, ch_name);
      _INS(sesso, cho);
    }
  return cho;
}

/**
 * Opens channel
 *
 * @brief x_session_channel_open
 * @param ctx
 * @param rjid
 * @param sid
 * @param ch_name
 * @return
 */
x_object *
x_session_channel_open(x_object *ctx, const char *rjid, const char *sid,
    const char *ch_name)
{
  x_obj_attr_t hints =
    { NULL, NULL, NULL, };

  x_object *sesso;
  x_object *cho;
  ENTER;

  TRACE("Opening channel '%s' in '%s' from '%s'\n", ch_name, sid, rjid);

  setattr("sid", (VAL) sid, &hints);
  sesso = X_OBJECT(x_session_open(rjid, ctx, &hints, 0));

  if (!sesso)
    {
//      x_object_print_path(ctx->bus, 0);
      BUG();
    }

  cho = x_session_channel_open2(sesso, ch_name);

  EXIT;
  return cho;
}

/**
 * @brief x_session_channel_close
 * @param cho Channel object
 * @return
 */
int
x_session_channel_close(x_object *cho)
{
    x_object_destroy_tree(cho);
}

/**
 * @brief x_session_channel_set_transport_profile_ns
 * @param cho Channel object
 * @param tname Name of transport class
 * @param ns Namespace name
 * @param hints Hints list
 * @return
 */
int
x_session_channel_set_transport_profile_ns(x_object *cho,
    const x_string_t tname, const x_string_t ns, x_obj_attr_t *hints)
{
  x_object *xobj;
  ENTER;

//  xobj = _GNEW(tname, ns);
  xobj = x_object_new_ns_match(tname, ns, hints, FALSE);
  if (!xobj)
  {
      TRACE("ERROR!! NOT FOUND! xobj(%s),ns(%s)\n",tname,ns);
      return -1;
  }

//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  _SETNM(xobj, "transport");
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);

  _ASGN(xobj, hints);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  _INS(cho, xobj);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);

  EXIT;
  return 0;
}

/**
 * @brief x_session_channel_set_media_profile_ns
 * @param cho
 * @param mname
 * @param ns
 * @return
 */
int
x_session_channel_set_media_profile_ns(x_object *cho, const x_string_t mname,
    const x_string_t ns)
{
  x_object *xobj = NULL;
  ENTER;
  xobj = _GNEW(mname, ns);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  _SETNM(xobj, MEDIA_IN_CLASS_STR);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  _INS(cho, xobj);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);

  xobj = _GNEW(mname, ns);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  _SETNM(xobj, MEDIA_OUT_CLASS_STR);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  _INS(cho, xobj);
//  TRACE("xobj(%p),objclass(%p)\n",xobj,xobj->objclass);
  EXIT;
  return 0;
}

/**
 * @brief x_session_channel_map_flow_twin - maps data flow sequence
 * @param cho
 * @param src
 * @param dst
 * @return 0 on success, -1 on error
 */
int
x_session_channel_map_flow_twin(
        x_object *cho, const x_string_t src,
        const x_string_t dst)
{
    x_object *to;
    x_object *from;

    ENTER;

    to =  _CHLD(cho,dst);
    from =  _CHLD(cho,src);

    // TODO map all objects with the same name
    x_object_set_write_handler(from , to);

    EXIT;

    return 0;
}


int
x_session_del_payload_from_channel(
        x_object *cho, const x_string_t ptype,
        const x_string_t ptname, int io_mod, x_obj_attr_t *_hints)
{

}


/**
 * Before calling this function you need to set 'mode'
 * attribute in '_hints' parameter to 'in' or 'out' depending on
 * operation mode of payload (decoder or encoder)
 *
 * @brief x_session_add_payload_to_channel
 * @param cho
 * @param ptype
 * @param ptname
 * @param io_mod
 * @param _hints
 * @return
 */
int
x_session_add_payload_to_channel(
        x_object *cho, const x_string_t ptype,
        const x_string_t ptname, int io_mod, x_obj_attr_t *_hints)
{
    x_object *xobj;
    x_object *tmpo;
    x_object *payloado;

    ENTER;

//    x_object_print_path(cho, 0);

    payloado = x_object_new_ns_match(ptname, _XS("gobee:media"), _hints, FALSE);

    if (!payloado)
    {
        TRACE("Class '%s' not found\n",ptname);
        return (-1);
    }
  
    TRACE("Setting up payload map (IO mod = %d)\n", MEDIA_IO_MODE_IN);
    _SETNM(payloado, ptype);

    // set operation mode
    _ASET(payloado, _XS("id"), ptype);
    _ASET(payloado, _XS("name"), ptname);

    if (io_mod==MEDIA_IO_MODE_IN)
    {
        _ASET(payloado, _XS(X_PRIV_STR_MODE), _XS("in"));
        xobj = _CHLD(cho,MEDIA_IN_CLASS_STR);
        if (!xobj)
        {
            ERROR;
            return (-1);
        }
        _INS(xobj, payloado);

        if(NEQ("HID",ptname)) // FIXME!! a.a.e UGLY hardcoded hack!
        {
            /* set write handler for decoders */
            tmpo = _CHLD(cho,_XS("dataplayer"));
            if (tmpo)
            {
                x_object_set_write_handler(payloado, tmpo);
                TRACE("Set up next hop for decoder\n");
            }
            else
                TRACE("No Data player set for the channel!!!!!!!!!!!!!!!!!!!!!!!\n");
        }
    }
    else
    {
        _ASET(payloado, _XS(X_PRIV_STR_MODE), _XS("out"));
        xobj = _CHLD(cho,MEDIA_OUT_CLASS_STR);
        if (!xobj)
        {
            ERROR;
            return (-1);
        }
        _INS(xobj, payloado);

        /* set write handler for encoders */
        tmpo = _CHLD(cho,_XS("transport"));
        if (tmpo)
        {
            x_object_set_write_handler(payloado, tmpo);
            TRACE("Set up next hop for encoder\n");
        }
        else
            TRACE("No transport set for the channel!!!!!!!!!!!!!!!!!!!!!!!\n");
    }

    setattr(X_PRIV_STR_MODE,NULL,_hints);
    _ASGN(payloado, _hints);

    EXIT;
    return 0;
}

#if 0
int
x_session_set_channel_io_mode(x_object *cho, int mode)
  {
    switch (mode)
      {
        case CHANNEL_MODE_IN:
        case CHANNEL_MODE_OUT:
        case CHANNEL_MODE_BOTH:
        default:
        break;
      };
  }
#endif

/**
 * @brief x_session_channel_add_transport_candidate
 * @param cho
 * @param cand
 */
void
x_session_channel_add_transport_candidate(x_object *cho, x_object *cand)
{
  ENTER;
  _INS(_CHLD(cho,"transport"), _CPY(cand));
  EXIT;
}

/**
 * @brief x_session_start
 * @param sess
 */
void
x_session_start(x_object *sess)
{
    x_obj_attr_t attrs = {0,0,0};
    ENTER;

    TRACE("Starting session\n");

    setattr( _XS("$state"),_XS("active"),&attrs);
    _ASGN(sess,&attrs);
    attr_list_clear(&attrs);

    EXIT;
}

/**
 * @brief x_session_stop
 * @param sess
 * @deprecated
 */
void
x_session_stop(x_object *sess)
{
    x_obj_attr_t attrs = {0,0,0};
    ENTER;

    TRACE("Stopping session\n");

    setattr( _XS("$state"),_XS("stopped"),&attrs);
    _ASGN(sess,&attrs);
    attr_list_clear(&attrs);

    EXIT;
}
