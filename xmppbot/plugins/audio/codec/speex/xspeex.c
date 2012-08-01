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
#include <xwlib/x_config.h>
#if TRACE_XAUDIO_SPEEX_ON
#define DEBUG_PRFX "[xSPEEX] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

//#include <ogg/ogg.h>
#include <speex/speex.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#define SPEEX_BRATE 16000
#define PCM_FRAME_LEN 160
#define SPEEX_FRAME_LEN 200
#define _MAX_SEC 100

#define SPEEX_MOD_ENC 0
#define SPEEX_MOD_DEC 1

enum
{
  SPEEX_BUS_APPENDED = 1, SPEEX_READY_OP, SPEEX_FINISHED,
};

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
  snd_pcm_t *snddev;
  int pcm_file;
//  void (*frame_ready_cb)(void *frame, int siz);
} xSPEEX_t;

#define FRAME_SIZE 160

/**
 * @brief Encodes 16 bit audio to speex codec.
 */
static int
x_speex_encode(xSPEEX_t *spxctx, void *data, int siz)
{
  int ready;

  speex_bits_reset(&spxctx->bits);

  ready = !speex_encode(spxctx->codecCtx, data, &spxctx->bits);
  //  if (ready)
  //    speex_bits_write(&spxctx->bits, spxctx, siz);

  /*
   if (ready)
   speex_bits_reset(&spxctx->bits);
   */

  return ready;
}

/**
 * @brief Creates new speex encoder state
 */
static int
x_speex_encoder_init(xSPEEX_t *speex)
{
  int tmp = 10;

  speex->codecCtx = speex_encoder_init(&speex_nb_mode);
  speex->mode = SPEEX_MOD_ENC;

  _ASET(X_OBJECT(speex), "clockrate", "16000");

  speex_encoder_ctl(speex->codecCtx, SPEEX_SET_QUALITY, &tmp);
  speex_encoder_ctl(speex->codecCtx, SPEEX_GET_BITRATE, &tmp);

  TRACE("SPEEX RATE=%d\n", tmp);

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
  if (spxctx->codecCtx)
    speex_encoder_destroy(spxctx->codecCtx);
  return;
}

/**
 * @brief Encodes 16 bit audio to speex codec.
 */
static int
x_speex_decode(xSPEEX_t *spxctx, void *data, int siz)
{
  int err = 0;
  int bs1, bs2;

  TRACE("Decoding %d bytes\n", siz);

  //  bs1 = open("pcm.bs1", O_APPEND | O_RDWR);
  //  write(bs1, spxctx->bits.chars, siz);
  //  close(bs1);

  if (spxctx->curframe)
    {
#if 1
      //      speex_bits_reset(&spxctx->bits);
      speex_bits_read_from(&spxctx->bits, data, siz);
#else
      speex_bits_reset(&spxctx->bits);
      speex_bits_set_bit_buffer(&spxctx->bits, data, siz);
#endif

      err = speex_decode_int(spxctx->codecCtx, &spxctx->bits,
          (short *) spxctx->curframe);

#ifdef DEBUG_DATA
      bs2 = open("xspeex-remote.pcm", O_APPEND | O_RDWR);
      write(bs2, spxctx->curframe,
          (spxctx->curfsamplesiz >> 3) * spxctx->curframesiz);
      close(bs2);
#endif
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
deleteMe_alsa_open(xSPEEX_t *mctl)
{
  long loops;
  int rc;
  int size;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;

  ENTER;

  /* Open PCM device for playback. */
  rc = snd_pcm_open(&mctl->snddev, "default", SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0)
    {
      fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
      exit(1);
    }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(mctl->snddev, params);

  /* Set the desired hardware parameters. */
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(mctl->snddev, params,
      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(mctl->snddev, params, SND_PCM_FORMAT_S16_LE);

  snd_pcm_hw_params_set_channels(mctl->snddev, params, 1);

  val = SPEEX_BRATE;
  //  snd_pcm_hw_params_set_rate_near(mctl->snddev, params, &val, &dir);
  snd_pcm_hw_params_set_rate_near(mctl->snddev, params, &val, &dir);

  /* Set period size to 32 frames. */
  frames = 0;
  snd_pcm_hw_params_set_period_size_near(mctl->snddev, params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(mctl->snddev, params);
  if (rc < 0)
    {
      fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
      exit(1);
    }

  snd_pcm_prepare(mctl->snddev);

  EXIT;
}

static int
x_speex_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
  int err;
  xSPEEX_t *mctl = (xSPEEX_t *) o;
  u_int32_t l;

  //  ENTER;
  //  TRACE("Recv %d bytes of data...\n", len);

  err = x_speex_decode(mctl, buf, len);
  if (!err)
    {
      // call to frame ready callback
      TRACE("Successfully decoded speech data...\n");
      if (!mctl->snddev)
        {
          deleteMe_alsa_open(mctl);
        }
      if (mctl->snddev)
        {
          err = snd_pcm_writei(mctl->snddev, mctl->curframe, mctl->curframesiz);
          TRACE("Wrote %d sound frames, recorded %d*%d bytes\n",
              err, mctl->curframesiz, mctl->curfsamplesiz >> 3);

#ifdef DEBUG_DATA
          mctl->pcm_file = open("xspeex-near.pcm", O_APPEND | O_RDWR);
          write(mctl->pcm_file, mctl->curframe,
              mctl->curframesiz * (mctl->curfsamplesiz >> 3));
          TRACE("Wrote %d sound frames, recorded %d*%d bytes\n",
              err, mctl->curframesiz, mctl->curfsamplesiz >> 3);
          close(mctl->pcm_file);
#endif
        }
    }
  else if (err = -2)
    {
      TRACE("Corrupted bitstream\n");
    }
  //  EXIT;
  return len;
}

static void
x_speex_chkstate(xSPEEX_t *spxctx)
{
  ENTER;
  EXIT;
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
  xSPEEX_t *pres = (xSPEEX_t *) (void *) so;
  ENTER;

  x_speex_decoder_init(pres);

  if (pres->state != SPEEX_FINISHED)
    {
      pres->state = SPEEX_BUS_APPENDED;
      pres->snddev = NULL;
      x_speex_chkstate(pres);
    }

  EXIT;
  return 0;
}

static x_object *
speex_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  xSPEEX_t *pres = (xSPEEX_t *) this__;
  ENTER;

  x_object_default_assign_cb(this__, attrs);

  x_speex_chkstate(pres);

  EXIT;
  return this__;
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
  _ASET(speex, _XS("name"), _XS("SPEEX"));
  _ASET(speex, _XS("description"), _XS("SPEEX audio codec"));
  _ASET(speex, _XS("classname"), _XS("SPEEX"));
  _ASET(speex, _XS("xmlns"), _XS("gobee:media"));

  EXIT;
}

static struct x_path_listener speex_bus_listener;

__x_plugin_visibility
__plugin_init void
x_speeex_init(void)
{
  ENTER;
  speex_classs.cname = "SPEEX";
  speex_classs.psize = (sizeof(xSPEEX_t) - sizeof(x_object));
  speex_classs.match = &speex_match;
  speex_classs.on_assign = &speex_on_assign;
  speex_classs.on_append = &speex_on_append;
  speex_classs.finalize = &speex_exit;
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

