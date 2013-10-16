/**
 * xspeex.c
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
#if TRACE_XAUDIO_SPEEX_ON
#define DEBUG_PRFX "[xSPEEX] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

//#include <ogg/ogg.h>
#include <speex/speex.h>

#define SPEEX_BRATE 16000
#define PCM_FRAME_LEN 320
#define SPEEX_FRAME_LEN 200
#define _MAX_SEC 100

enum
{
    TYPE_UNSPECIFIED = -1,
    SPEEX_MOD_ENC = 1,
    SPEEX_MOD_DEC = 2,
};

enum
{
    SPEEX_BUS_APPENDED = 1, SPEEX_READY_OP, SPEEX_FINISHED,
};

#define FRAME_SIZE 160

typedef struct xSPEEX
{
    x_object obj;
    SpeexBits bits;
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
} xSPEEX_t;

/**
 * @brief Encodes 16 bit audio to speex codec.
 */
static int
x_speex_encode(xSPEEX_t *thiz, void *data, int siz)
{
    int err = -1;
    int ready;

//    do {
//      int bs2;
//      //        #ifdef DEBUG_DATA
//      bs2 = open("xspeex-near.pcm", O_APPEND | O_RDWR);
//      write(bs2, data, siz);
//      close(bs2);
//      //        #endif
//    } while (0);

    speex_bits_reset(&thiz->bits);

    ready = speex_encode_int(thiz->codecCtx, data, &thiz->bits);

    if (ready)
    {
        TRACE("Frame ready.... available... %d bytes\n", speex_bits_nbytes(&thiz->bits));

        err = speex_bits_write(&thiz->bits,
                               thiz->curframe,
                               (thiz->curfsamplesiz >> 3) * PCM_FRAME_LEN);
        TRACE("Frame ready.... writing... %d bytes\n", err);
    }
    else
    {
        TRACE("Frame not ready....\n");
    }

#if 0
    for(i = 0; i < (inputFrames / frameSize); i++){
        speex_bits_reset(&bits);
        speex_encode_int(state, &input[i * frameSize], &bits);
        bufferLength = speex_bits_nbytes(&bits);
        output = realloc(output, outputLength + bufferLength);
        speex_bits_write(&bits, (char*)(output + outputLength), bufferLength);
        outputLength += bufferLength;
    }
#endif
    return err;
}

/**
 * @brief Creates new speex encoder state
 */
static int
x_speex_encoder_init(xSPEEX_t *speex)
{
    int tmp = 10;

//    speex->curframesiz = PCM_FRAME_LEN;

    speex->codecCtx = speex_encoder_init(&speex_wb_mode);
    speex->mode = SPEEX_MOD_ENC;

    _ASET(X_OBJECT(speex), "clockrate", "16000");

    speex_encoder_ctl(speex->codecCtx, SPEEX_SET_QUALITY, &tmp);

    tmp = 30000;
    speex_encoder_ctl(speex->codecCtx, SPEEX_SET_BITRATE, &tmp);
    speex_encoder_ctl(speex->codecCtx, SPEEX_GET_FRAME_SIZE, &speex->curframesiz);

    tmp = 16000;
    speex_encoder_ctl(speex->codecCtx, SPEEX_SET_SAMPLING_RATE, &tmp);

    TRACE("SPEEX RATE=%d, frame_size=%d\n", tmp, speex->curframesiz);

    speex->curfsamplesiz = 16;
    speex->curframe =
            x_malloc((speex->curfsamplesiz >> 3) * PCM_FRAME_LEN);
    speex_bits_init(&speex->bits);

    return 0;
}

/**
 * @brief Deletes speex encoder state
 */
static void
x_speex_encoder_destroy(xSPEEX_t *spxctx)
{
    speex_bits_destroy(&spxctx->bits);
    if (spxctx->curframe)
        x_free(spxctx->curframe);

    if (spxctx->codecCtx)
        speex_encoder_destroy(spxctx->codecCtx);
    return;
}

/**
 * @brief Encodes 16 bit audio to speex codec.
 */
static int
x_speex_decode(xSPEEX_t *thiz, void *data, int siz)
{
    int err = 0;

    TRACE("Decoding %d bytes\n", siz);

    //  bs1 = open("pcm.bs1", O_APPEND | O_RDWR);
    //  write(bs1, spxctx->bits.chars, siz);
    //  close(bs1);

    if (thiz->curframe)
    {
#if 1
        speex_bits_reset(&thiz->bits);
        speex_bits_read_from(&thiz->bits, data, siz);
#else
        speex_bits_reset(&spxctx->bits);
        speex_bits_set_bit_buffer(&spxctx->bits, data, siz);
#endif

        err = speex_decode_int(thiz->codecCtx, &thiz->bits,
                               (short *) thiz->curframe);

    }

    return err;
}

/**
 * @brief Creates new speex encoder state
 */
static int
x_speex_decoder_init(xSPEEX_t *speex)
{
    int tmp = 10;
    int rate;

    speex->codecCtx = speex_decoder_init(&speex_wb_mode);
    speex->mode = SPEEX_MOD_DEC;

    _ASET(X_OBJECT(speex), "clockrate", "16000");
    //  rate = atoi(_AGET(X_OBJECT(speex),"clockrate"));
    rate = 16000;

    speex->curfsamplesiz = 16;

    speex_decoder_ctl(speex->codecCtx, SPEEX_RESET_STATE, NULL);
    //  speex_decoder_ctl(speex->codecCtx, SPEEX_SET_QUALITY, &tmp);
    speex_decoder_ctl(speex->codecCtx, SPEEX_SET_SAMPLING_RATE, &rate);
    speex_decoder_ctl(speex->codecCtx, SPEEX_GET_FRAME_SIZE, &speex->curframesiz);

    TRACE("SPEEX FRAME SIZE=%d\n", speex->curframesiz);

    speex->curframe = x_malloc((speex->curfsamplesiz >> 3) * speex->curframesiz);
    speex_bits_init(&speex->bits);

    return 0;
}

/**
 * @brief Deletes speex encoder state
 */
static void
x_speex_decoder_destroy(xSPEEX_t *spxctx)
{
    speex_bits_destroy(&spxctx->bits);
    if (spxctx->curframe)
        x_free(spxctx->curframe);

    if (spxctx->codecCtx)
        speex_decoder_destroy(spxctx->codecCtx);
    return;
}


static int
x_speex_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    u_int32_t l;
    xSPEEX_t *thiz = (xSPEEX_t *) o;
    x_obj_attr_t hints = { 0, 0, 0 };

    TRACE("\n");

    switch (thiz->mode)
    {
    case SPEEX_MOD_DEC:
        TRACE("Push packet to decode len=%d\n", len);
        err = x_speex_decode(thiz, buf, len);
        if (!err)
        {
            _WRITE(o->write_handler, thiz->curframe,
                   (thiz->curfsamplesiz >> 3) * thiz->curframesiz, NULL);
          
          TRACE("Wrote frame of len=%d to '%s'\n", (thiz->curfsamplesiz >> 3) * thiz->curframesiz,
                _GETNM(o->write_handler));

          do {
            int bs1, bs2;
            //        #ifdef DEBUG_DATA
            bs2 = open("xspeex-remote.pcm", O_APPEND | O_RDWR);
            write(bs2, thiz->curframe,
                  (thiz->curfsamplesiz >> 3) * thiz->curframesiz);
            close(bs2);
            //        #endif
          } while (0);
          
        }
        break;

    case SPEEX_MOD_ENC:
        TRACE("Push packet to encode len=%d\n", len);
        err = x_speex_encode(thiz, buf, len);
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

            TRACE("Encoded %d bytes", err);

            _WRITE(thiz->obj.write_handler, thiz->curframe, err, &hints);
            attr_list_clear(&hints);
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
speex_match(x_object *o, x_obj_attr_t *_o)
{
    /*const char *xmlns;*/
    ENTER;

    EXIT;
    return TRUE;
}

static int
speex_on_append(x_object *so, x_object *parent)
{
    xSPEEX_t *thiz = (xSPEEX_t *) (void *) so;

    ENTER;
    EXIT;
    return 0;
}

static x_object *
speex_on_assign(x_object *so, x_obj_attr_t *attrs)
{
    xSPEEX_t *thiz = (xSPEEX_t *) so;
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
                thiz->mode = SPEEX_MOD_DEC;
            }
            else if (md && EQ(md,"out"))
            {
                thiz->mode = SPEEX_MOD_ENC;
            }
        }
    }

    TRACE("Codec mode %s\n", md);

    x_object_default_assign_cb(so, attrs);

    switch (thiz->mode)
    {
    case SPEEX_MOD_DEC:
        x_speex_decoder_init(thiz);
        break;

    case SPEEX_MOD_ENC:
        x_speex_encoder_init(thiz);
        break;
    };


    EXIT;
    return so;
}

static void
speex_exit(x_object *this__)
{
    ENTER;
    printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
    EXIT;
}

static struct xobjectclass speex_classs;

static void
___presence_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *speex;

    ENTER;

    speex = x_object_add_path(obj, "__proc/audio/SPEEX", NULL);
    _ASET(speex, _XS("name"), _XS("speex"));
    _ASET(speex, _XS("description"), _XS("SPEEX audio codec"));
    _ASET(speex, _XS("classname"), _XS("speex"));
    _ASET(speex, _XS("xmlns"), _XS("gobee:media"));

    EXIT;
}

static struct x_path_listener speex_bus_listener;

__x_plugin_visibility
__plugin_init void
x_speeex_init(void)
{
    ENTER;
    speex_classs.cname = "speex";
    speex_classs.psize = (sizeof(xSPEEX_t) - sizeof(x_object));
    speex_classs.match = &speex_match;
    speex_classs.on_assign = &speex_on_assign;
    speex_classs.on_append = &speex_on_append;
    speex_classs.commit = &speex_exit;
    speex_classs.try_write = &x_speex_try_write;

    x_class_register_ns(speex_classs.cname, &speex_classs, _XS("gobee:media"));

    speex_bus_listener.on_x_path_event = &___presence_on_x_path_event;

    /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
    x_path_listener_add("/", &speex_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_speeex_init)
;

