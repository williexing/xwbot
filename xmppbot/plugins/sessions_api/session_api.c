/**
 * session_api.c
 *
 *  Created on: Mar 21, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <xwlib/x_config.h>
#if TRACE_XSESSIONS_API_ON
#define DEBUG_PRFX "[sessions_api] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>
#include "sessions.h"

#if 0
static void
__session_system_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
  {
    ssize_t len;
    socklen_t slen;
    x_common_session *xcs = (x_common_session *) (((char *) watcher)
        - offsetof(x_common_session, watcher));
    char buf[1024];

    ENTER;

    if (xcs /* && (revents & EV_READ) */)
      {
        TRACE("Read event\n");
        if (!xcs->ioset[XSESS_IO_MSG].io_data)
        xcs->ioset[XSESS_IO_MSG].io_data = malloc(sizeof(struct sockaddr_un));

        BUG_ON(!xcs->ioset[XSESS_IO_MSG].io_data);
        memset(xcs->ioset[XSESS_IO_MSG].io_data, 0, sizeof(struct sockaddr_un));

#if 0
        if ((len = x_read(xcs->ioset[XSESS_IO_MSG].sysfd, buf, 1024)) > 0)
#else
        if ((len = recvfrom(xcs->ioset[XSESS_IO_MSG].sysfd, buf, sizeof(buf), 0,
                    (struct sockaddr *) xcs->ioset[XSESS_IO_MSG].io_data, &slen)) > 0)
#endif
          {
            TRACE("Read from IO (%d)\n", xcs->ioset[XSESS_IO_MSG].sysfd);

            if (xcs->xobj.write_handler)
            x_object_try_write(xcs->xobj.write_handler, buf, len, NULL);

          }
        else if (len == 0)
          {
            TRACE("Need to reopen I/O pipe.\n");
          }
        else
          {
            perror("ERROR");
          }
      }

    EXIT;
  }

/*
 * Creates external IO communication slot.
 */
#if 1
static int
__session_create_io_slot(const char *slotname)
  {
    int sockfd;
    struct sockaddr_un server_address;

    ENTER;

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0)
      {
        TRACE("server: opening socket failed\n");
        BUG();
        EXIT;
        return -1;
      }

    x_memset(&server_address, 0, sizeof(server_address));

    server_address.sun_family = AF_UNIX;
    x_strncpy(server_address.sun_path, slotname, sizeof(server_address.sun_path)
        - 1);
    unlink(slotname);

    if (bind(sockfd, (struct sockaddr *) &server_address, sizeof(server_address))
        == -1)
      {
        close(sockfd);
        TRACE("server: bind failed on %s\n", slotname);
        BUG();
        EXIT;
        return -1;
      }

    /* switch it to async nonblocking operation mode */
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    fcntl(sockfd, F_SETFL, O_ASYNC);

    EXIT;
    return sockfd;
  }
#else
static int
__session_create_io_slot(const char *slotname)
  {
    int sockfd;
    sockfd = open(slotname, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
    return sockfd;
  }
#endif

/**
 * Creates new system IO session reference
 * TODO This should be called from x_open_session() call.
 */
static void UNUSED
___session_create_system_ref(x_common_session *xsess)
  {
    const char *sid = x_object_getattr(X_OBJECT(xsess), "sid");
    char *rjid;
    char *strbuf;
    ENTER;

    strbuf = (char *) x_object_getattr(X_OBJECT(xsess), "remote");
    BUG_ON(!strbuf);
    JID_TO_SID_A(rjid,strbuf);

    strbuf
    = alloca(x_strlen(rjid) + x_strlen(sid) + x_strlen(SESSION_WORKING_DIRECTORY) + 2);
    sprintf(strbuf, SESSION_WORKING_DIRECTORY "/%s", rjid);

    x_mkdir_p(strbuf);

    sprintf(strbuf, SESSION_WORKING_DIRECTORY "/%s/%s", rjid, sid);
    xsess->ioset[XSESS_IO_MSG].sysfd = __session_create_io_slot(strbuf);

    ev_io_init(
        &xsess->watcher,
        __session_system_cb,
        xsess->ioset[XSESS_IO_MSG].sysfd,
        EV_READ);

    EXIT;
    return;
  }
#endif

/***********************************/
/***********************************/
/* SESSION INTERNAL API      */
/***********************************/
/***********************************/
static x_common_session *
x_session_find(x_object *root, x_obj_attr_t *attrs)
{
  x_object *_o = NULL;
  ENTER;
  _o = _FIND(_CHLD(root,"__isession"), attrs);
  EXIT;
  return (x_common_session *) _o;
}

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
      _o = _NEW(sbuf,"gobee");
      _INS(_sesso, _o);
    }

  if (!xcs && (flags == X_CREAT))
    {
      xcs = (x_common_session *) _NEW("__isession", "jingle");
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
 * Opens new session channel.
 * This will open new communication channel with
 * channel number number in given session context.
 *
 * @param sess Session handler.
 * @param iod Channel number
 * @param flags flags
 */
int
x_session_open_io(x_common_session *sess, int iod, int flags)
{
  ENTER;
  if (iod < 0 || iod >= MAX_FDS_PER_SESSION)
    return -1;
  EXIT;
  return iod;
}

/**
 * Closes session channel.
 *
 * @param sess Session handler.
 * @param iod Channel number
 * @param flags flags
 */
void
x_session_close_io(x_common_session *sess, int iod)
{
  ENTER;
  EXIT;
}

int
x_session_read_io(x_common_session *sess, int iod, char *buf, size_t siz)
{
  int err;
  ENTER;

  err = x_read(sess->ioset[iod].sysfd, buf, siz);

  EXIT;
  return err;
}

int
x_session_write_io(x_common_session *sess, int iod, char *buf, size_t siz)
{
  int err = -1;
  ENTER;

#if 1
  err = x_write(sess->ioset[iod].sysfd, buf, siz);
#else
    {
      struct sockaddr_un *clia;
      if (sess->ioset[iod].io_data)
        {
          clia = (struct sockaddr_un *) sess->ioset[iod].io_data;
          err = sendto(sess->ioset[iod].sysfd, buf, siz, 0,
              (struct sockaddr *) clia, sizeof(*clia));
        }
    }
#endif

  TRACE("+++++++++ %d Writing %s to %d, siz = %d\n",
      err, buf, sess->ioset[iod].sysfd, (int) siz);
  EXIT;
  return err;
}

/**
 * Registers write callback for given session for given communication
 * channel.
 */
void
x_session_register_ioread_callback(x_common_session *sess, int iod,
    x_session_write_cb_f *cb)
{
  ENTER;
  sess->ioset[iod].write_up = cb;
  EXIT;
}

x_object *
x_session_channel_open2(x_object *sesso, const char *ch_name)
{
  x_object *cho;
  cho = _CHLD(sesso, ch_name);
  if (!cho)
    {
      cho = _NEW(_XS("__iostream"),_XS("jingle"));
      _SETNM(cho, ch_name);
      _INS(sesso, cho);
    }
  return cho;
}

/**
 * Opens channel
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
      x_object_print_path(ctx->bus, 0);
      BUG();
    }

  cho = x_session_channel_open2(sesso, ch_name);

  EXIT;
  return cho;
}

/**
 *
 */
int
x_session_channel_set_transport_profile(x_object *cho, const char *tname,
    x_obj_attr_t *hints)
{
  x_object *xobj;
  ENTER;
  xobj = _NEW(tname, "jingle");
  _SETNM(xobj, "transport");
  _ASGN(xobj, hints);
  _INS(cho, xobj);
  EXIT;
  return 0;
}

/**
 *
 */
int
x_session_channel_set_media_profile(x_object *cho, const char *mname)
{
  x_object *xobj;
  ENTER;
  xobj = _NEW(mname, "jingle");
  _SETNM(xobj, MEDIA_IN_CLASS_STR);
  _INS(cho, xobj);

  xobj = _NEW(mname, "jingle");
  _SETNM(xobj, MEDIA_OUT_CLASS_STR);
  _INS(cho, xobj);
  EXIT;
  return 0;
}

int
x_session_add_payload_to_channel(x_object *cho, const x_string_t ptype,
    const x_string_t ptname, x_obj_attr_t *_hints)
{
  x_object *xobj,*m_out;
  x_object *tmpo;
  x_object *payloado;
  x_obj_attr_t hints =
    { NULL, NULL, NULL, };
  ENTER;

  x_object_print_path(cho,0);

  payloado = x_object_new_ns_match(ptname, _XS("gobee:media"), NULL, FALSE);

  if (!payloado)
    return (-1);

  /* add decoders */
  TRACE("Setting up decoders\n");
  xobj = _CHLD(cho,MEDIA_IN_CLASS_STR);
  _SETNM(payloado, ptype);
  // set operation mode
  _ASET(payloado,_XS("mode"), _XS("in"));
  _INS(xobj, payloado);
  _ASGN(payloado, _hints);

#if 1
  /* set write handler for decoders */
  tmpo = _CHLD(cho,_XS("dataplayer"));
  if (tmpo)
    {
      x_object_set_write_handler(payloado, tmpo);
      TRACE("Set up next hop for decoder\n");
    }
  else
    TRACE("No Data player set for the channel!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif

  /* add encoders */
  TRACE("Setting up encoders\n");
  payloado = x_object_new_ns_match(ptname, _XS("gobee:media"), NULL, FALSE);
  m_out = _CHLD(cho,MEDIA_OUT_CLASS_STR);
  if (!m_out)
    {
      ERROR;
      return (-1);
    }
  _SETNM(payloado,ptype);
  // set operation mode
  _ASET(payloado,_XS("mode"), _XS("out"));
  _INS(m_out, payloado);
  _ASGN(payloado, _hints);

  /* set write handler for decoders */
  tmpo = _CHLD(cho,_XS("transport"));
  if (tmpo)
    {
      x_object_set_write_handler(payloado, tmpo);
      TRACE("Set up next hop for encoder\n");
    }
  else
    TRACE("No transport set for the channel!!!!!!!!!!!!!!!!!!!!!!!\n");

  EXIT;

  return 0;
}

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

void
x_session_channel_add_transport_candidate(x_object *cho, x_object *cand)
{
  ENTER;
  _INS(_CHLD(cho,"transport"), _CPY(cand));
  EXIT;
}
