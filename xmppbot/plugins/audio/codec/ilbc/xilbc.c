/**
 * xilbc.c
 *
 *  Created on: Apr 3, 2012
 *      Author: artemka
 */

/*
 * bind.c
 *
 *  Created on: Jul 28, 2011
 *      Author: artemka
 */
#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XAUDIO_ILBC_ON
#define DEBUG_PRFX "[xILBC] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

//#include <xilbc/xilbc.h>

#define ILBC_BRATE 16000
#define PCM_FRAME_LEN 160
#define ILBC_FRAME_LEN 200
#define _MAX_SEC 100

enum
{
    TYPE_UNSPECIFIED = -1,
    ILBC_MOD_ENC = 1,
    ILBC_MOD_DEC = 2,
};

enum
{
    ILBC_BUS_APPENDED = 1, ILBC_READY_OP, ILBC_FINISHED,
};

#define FRAME_SIZE 160

typedef struct xILBC
{
    x_object obj;
    void *codecCtx;
    void *curframe;
    int curframesiz;
    int curfsamplesiz;
    int mode;
    int state;
    int pcm_file;

    unsigned int seqno;
    time_t stamp;
    //  void (*frame_ready_cb)(void *frame, int siz);
} xILBC_t;

/**
 * @brief Encodes 16 bit audio to xilbc codec.
 */
static int
x_xilbc_encode(xILBC_t *thiz, void *data, int siz)
{
    int err = -1;
    int ready;
    return err;
}

/**
 * @brief Creates new xilbc encoder state
 */
static int
x_xilbc_encoder_init(xILBC_t *xilbc)
{
    int tmp = 10;

    // xilbc->codecCtx = xilbc_encoder_init(&xilbc_nb_mode);
    xilbc->mode = ILBC_MOD_ENC;

    _ASET(X_OBJECT(xilbc), "clockrate", "16000");

    xilbc->curfsamplesiz = 16;
    xilbc->curframe =
            x_malloc((xilbc->curfsamplesiz >> 3) * 320);

    return 0;
}

/**
 * @brief Deletes xilbc encoder state
 */
static void
x_xilbc_encoder_destroy(xILBC_t *spxctx)
{
    if (spxctx->curframe)
        x_free(spxctx->curframe);

    if (spxctx->codecCtx)
        ; // xilbc_encoder_destroy(spxctx->codecCtx);
    return;
}

/**
 * @brief Encodes 16 bit audio to xilbc codec.
 */
static int
x_xilbc_decode(xILBC_t *thiz, void *data, int siz)
{
    int err = -1;
    int bs1, bs2;

    TRACE("Decoding %d bytes\n", siz);

    //  bs1 = open("pcm.bs1", O_APPEND | O_RDWR);
    //  write(bs1, spxctx->bits.chars, siz);
    //  close(bs1);

    if (thiz->curframe)
    {
        //#ifdef DEBUG_DATA
        bs2 = open("xxilbc-remote.pcm", O_APPEND | O_RDWR);
        write(bs2, thiz->curframe,
              (thiz->curfsamplesiz >> 3) * thiz->curframesiz);
        close(bs2);
        //#endif
    }

    return err;
}

/**
 * @brief Creates new xilbc encoder state
 */
static int
x_xilbc_decoder_init(xILBC_t *xilbc)
{
    int tmp = 10;
    int rate;

    // xilbc->codecCtx = xilbc_decoder_init(&xilbc_wb_mode);
    xilbc->codecCtx = 0;
    xilbc->mode = ILBC_MOD_DEC;

    _ASET(X_OBJECT(xilbc), "clockrate", "16000");
    //  rate = atoi(_AGET(X_OBJECT(xilbc),"clockrate"));
    rate = 16000;

    xilbc->curfsamplesiz = 16;

    TRACE("ILBC FRAME SIZE=%d\n", xilbc->curframesiz);

    xilbc->curframe = x_malloc((xilbc->curfsamplesiz >> 3) * xilbc->curframesiz);

    return 0;
}

/**
 * @brief Deletes xilbc encoder state
 */
static void
x_xilbc_decoder_destroy(xILBC_t *spxctx)
{
    if (spxctx->curframe)
        x_free(spxctx->curframe);

    if (spxctx->codecCtx)
        ; // xilbc_decoder_destroy(spxctx->codecCtx);
    return;
}


static int
x_xilbc_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    u_int32_t l;
    xILBC_t *thiz = (xILBC_t *) o;
    x_obj_attr_t hints = { 0, 0, 0 };

    TRACE("\n");

    switch (thiz->mode)
    {
    case ILBC_MOD_DEC:
        TRACE("Push packet to decode len=%d\n", len);
        err = x_xilbc_decode(thiz, buf, len);
        if (!err)
        {
            _WRITE(o->write_handler, thiz->curframe,
                   (thiz->curfsamplesiz >> 3) * thiz->curframesiz, NULL);
        }
        break;

    case ILBC_MOD_ENC:
        TRACE("Push packet to encode len=%d\n", len);
        err = x_xilbc_encode(thiz, buf, len);
        if (err > 0)
        {
            setattr("pt", _GETNM(o), &hints);
            sprintf(buf, "%d", thiz->stamp);
            setattr("timestamp", buf, &hints);
            sprintf(buf, "%u", ++thiz->seqno);
            setattr("seqno", buf, &hints);

            thiz->stamp += 320;
            TRACE("writing to %p(%s)...\n",
                  thiz->obj.write_handler,
                  thiz->obj.write_handler ? _GETNM(thiz->obj.write_handler) : "null");

            TRACE("Encoded %d bytes\n", err);

            _WRITE(thiz->obj.write_handler, thiz->curframe, err, &hints);
            TRACE("finished\n");
        }
        else
        {
            TRACE("Error encoding data\n", len);
        }
        break;

    default:
        TRACE("Unknown codec mode\n");
        break;
    };

    //  EXIT;
    return len;
}

/* matching function */
static BOOL
xilbc_match(x_object *o, x_obj_attr_t *_o)
{
    /*const char *xmlns;*/
    ENTER;

    EXIT;
    return TRUE;
}

static int
xilbc_on_append(x_object *so, x_object *parent)
{
    xILBC_t *thiz = (xILBC_t *) (void *) so;

    ENTER;
    EXIT;
    return 0;
}

static x_object *
xilbc_on_assign(x_object *so, x_obj_attr_t *attrs)
{
    xILBC_t *thiz = (xILBC_t *) so;
    x_string_t md = getattr(X_PRIV_STR_MODE, attrs);
    ENTER;


    if (!thiz->mode || TYPE_UNSPECIFIED == thiz->mode)
    {
        if (!md)
        {
            md = _AGET(so,_XS(X_PRIV_STR_MODE));
            TRACE("Mode=%s\n",md);
        }

        if (!md)
        {
            TRACE("ERROR: Invalid codec state..\n");
        }
        else
        {
            if (md && EQ(md,"in"))
            {
                thiz->mode = ILBC_MOD_DEC;
            }
            else if (md && EQ(md,"out"))
            {
                thiz->mode = ILBC_MOD_ENC;
            }
        }
    }

    TRACE("Codec mode %s\n", md);

    x_object_default_assign_cb(so, attrs);

    switch (thiz->mode)
    {
    case ILBC_MOD_DEC:
        x_xilbc_decoder_init(thiz);
        break;

    case ILBC_MOD_ENC:
        x_xilbc_encoder_init(thiz);
        break;
    };


    EXIT;
    return so;
}

static void
xilbc_exit(x_object *this__)
{
    ENTER;
    printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
    EXIT;
}

static struct xobjectclass xilbc_classs;

static void
___presence_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *xilbc;

    ENTER;

    xilbc = x_object_add_path(obj, "__proc/audio/ILBC", NULL);
    _ASET(xilbc, _XS("name"), _XS("xilbc"));
    _ASET(xilbc, _XS("description"), _XS("ILBC audio codec"));
    _ASET(xilbc, _XS("classname"), _XS("xilbc"));
    _ASET(xilbc, _XS("xmlns"), _XS("gobee:media"));

    EXIT;
}

static struct x_path_listener xilbc_bus_listener;

__x_plugin_visibility
__plugin_init void
x_ilbc_init(void)
{
    ENTER;
    xilbc_classs.cname = "ilbc";
    xilbc_classs.psize = (sizeof(xILBC_t) - sizeof(x_object));
    xilbc_classs.match = &xilbc_match;
    xilbc_classs.on_assign = &xilbc_on_assign;
    xilbc_classs.on_append = &xilbc_on_append;
    xilbc_classs.commit = &xilbc_exit;
    xilbc_classs.try_write = &x_xilbc_try_write;

    x_class_register_ns(xilbc_classs.cname, &xilbc_classs, _XS("gobee:media"));

    xilbc_bus_listener.on_x_path_event = &___presence_on_x_path_event;

    /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
    x_path_listener_add("/", &xilbc_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_ilbc_init)
;

