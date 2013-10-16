/**
 * streams.c
 *
 *  Created on: Aug 13, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XSESSIONS_ON
#define DEBUG_PRFX "[iostreams] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>
#include "sessions.h"

/* matching function */
static void
__iostream_update_state(x_object *chan)
{
    x_object *tmpo;
    x_object *msg;
    x_io_stream *stream_ = (x_io_stream *) chan;
    x_obj_attr_t hints =
    { 0, 0, 0, };

    if ((stream_->regs.state & 0x3) == CHANNEL_STATE_READY)
        return;

    if (!(stream_->regs.state & CHANNEL_STATE_TRANSPORT_READY))
    {
        // check if transport ready
        if ((tmpo = _CHLD(chan,_XS("transport"))) && _CRGET(tmpo))
        {
            // transport ok!
            stream_->regs.state |= CHANNEL_STATE_TRANSPORT_READY;
        }
    }

    if (!(stream_->regs.state & CHANNEL_STATE_MEDIA_READY))
    {
        if ((tmpo = _CHLD(chan,MEDIA_IN_CLASS_STR)) && _CRGET(tmpo))
        {
            // transport ok!
            stream_->regs.state |= CHANNEL_STATE_MEDIA_READY;
        }
        if ((tmpo = _CHLD(chan,MEDIA_IN_CLASS_STR)) && _CRGET(tmpo))
        {
            // transport ok!
            stream_->regs.state |= CHANNEL_STATE_MEDIA_READY;
        }
    }

    if ((stream_->regs.state & 0x3) == CHANNEL_STATE_READY)
    {
        // set channel object ready
        _CRSET(chan, 1);

        TRACE("Channel ready: 0x%x\n",stream_->regs.state);
//        x_object_print_path(X_OBJECT(stream_),0);

#if 0
        // do notify
        msg = _GNEW(_XS("$message"),NULL);
        _ASET(msg, _XS("subject"), _XS("channel-ready"));
        _ASET(msg, _XS("chname"), _GETNM(chan));
        TRACE("CHANNEL READY '%s'\n", _GETNM(chan));
        x_object_send_down(_PARNT(chan), msg, &hints);
        _REFPUT(msg,NULl);
#endif
    }
    else
    {
        TRACE("Channel not ready: 0x%x\n",stream_->regs.state);
//        x_object_print_path(X_OBJECT(stream_),0);
    }
}

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
    x_object *transport;
    x_object *media;
    x_string_t _chnam_ = _GETNM(child);
    x_io_stream *stream_ = (x_io_stream *) chan;

    TRACE("-->> New child object appended '%s' <<--\n", _GETNM(child));

    //  __iostream_update_state(chan);
}

static x_object *
iostream_on_assign(x_object *chan, x_obj_attr_t *attrs)
{
    x_object *tmpo;
    x_string_t attr;

    ENTER;

    TRACE("\n");

    x_object_default_assign_cb(chan, attrs);

//    x_object_print_path(chan,1);

    if ((attr = getattr("mtype", attrs)))
    {
        if (EQ(attr,_XS("video")))
        {
            tmpo = _GNEW(_XS("video_player"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("dataplayer"));
            _INS(chan, tmpo);

            tmpo = _GNEW(_XS("camera"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("datasrc"));
            _INS(chan, tmpo);

            /**
             * Always try to add HID control:
             */
            tmpo = _GNEW(_XS("hiddemux"),_XS("gobee:media"));
            if (tmpo) _INS(chan, tmpo);

            tmpo = _GNEW(_XS("hidmux"),_XS("gobee:media"));
            if (tmpo) _INS(chan, tmpo);

        }
        else if (EQ(attr,_XS("audio")))
        {
            tmpo = _GNEW(_XS("audio_player"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("dataplayer"));
            _INS(chan, tmpo);

            tmpo = _GNEW(_XS("microphone"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("datasrc"));
            _INS(chan, tmpo);
        }
#if 0
        else if (EQ(attr,_XS("application")))
        {
            tmpo = _GNEW(_XS("hiddemux"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("dataplayer"));
            _INS(chan, tmpo);

            tmpo = _GNEW(_XS("hidmux"),_XS("gobee:media"));
            _SETNM(tmpo, _XS("datasrc"));
            _INS(chan, tmpo);
        }
#endif
//        else if (EQ(attr,_XS("message")))
//        {
//            TRACE("Setting 'message' session\n");
//            tmpo = _GNEW(_XS("textin"),_XS("gobee:media"));
//            _SETNM(tmpo, _XS("dataplayer"));
//            _INS(chan, tmpo);

//            tmpo = _GNEW(_XS("textout"),_XS("gobee:media"));
//            _SETNM(tmpo, _XS("datasrc"));
//            _INS(chan, tmpo);
//        }
    }

    if ((attr = getattr("$commit", attrs)))
    {
        // remove commit
        _ASET(chan, _XS("$commit"), NULL);
        __iostream_update_state(chan);
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

static int
iostream_msg_recv(x_object *thiz, const x_object *from, const x_object *msg)
{
    const char *chnam;
    x_string_t subj;
    struct jingle_object *jnglo = (struct jingle_object *) (void *) thiz;
    ENTER;

    subj = _AGET(msg,_XS("subject"));

    if (EQ(_XS("transport-ready"),subj))
    {
    }

    EXIT;
    return 0;
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
    __iostream_objclass.rx = &iostream_msg_recv;

    x_class_register_ns(__iostream_objclass.cname, &__iostream_objclass,
                        "jingle");

    EXIT;
    return;
}
PLUGIN_INIT(__iostream_init);

