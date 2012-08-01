/**
 * x_vpx_plugin.c
 *
 *  Created on: Feb 20, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[x_vpx] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include "x_vp8.h"

//                           1 0
// +----------------------------+
// |                        |MOD|
// +----------------------------+
typedef enum
{
  xVPX_NO_MOD = 0x0,
  xVPX_ENCODER_MOD = 0x1,
  xVPX_DECODER_MOD = 0x2,
  xVPX_INVALID_MOD = 0x3,

  xVPX_HAVE_W = 0x4,
  xVPX_HAVE_H = 0x8,
  //  xVPX_HAVE_RATE = 0x10,
  //  xVPX_HAVE_KEY_MIN = 0x20,
  //  xVPX_HAVE_KEY_MAX = 0x40,

  xVPX_INITIALIZED = 0x10000,
} xVPX_CR0_STATE;

#define xVPX_IS_CONFIGURED(x) \
  ((xVPX_HAVE_W | xVPX_HAVE_H) \
      == (x & (xVPX_HAVE_W | xVPX_HAVE_H)))
#define xVPX_IS_INITIALIZED(x) (x & xVPX_INITIALIZED)
#define xVPX_IS_ENCODER(x) (xVPX_ENCODER_MOD == (x & 0x3))
#define xVPX_IS_DECODER(x) (xVPX_DECODER_MOD == (x & 0x3))
#define xVPX_IS_INVALID(x) (xVPX_INVALID_MOD == (x & 0x3))
#define xVPX_IS_NO_MOD(x) (!(x & 0x3))

typedef struct
{
  x_object xobj;
  vpx_codec_ctx_t vpx_codec;
  vpx_codec_enc_cfg_t cfg;
  struct
  {
    int state;
    int id_counter;
    u_int32_t cr0;
  } regs;
} x_vpx_object_t;

static BOOL
x_vpx_equals(x_object *o, x_obj_attr_t *_o)
{
  ENTER;
  EXIT;
  return TRUE;
}

static int
x_vpx_on_append(x_object *so, x_object *parent)
{
  ENTER;
  EXIT;
  return 0;
}

static x_object *
x_vpx_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  register x_obj_attr_t *attr;
  x_vpx_object_t *x_codec = (x_vpx_object_t *) o;

  ENTER;

  for (attr = attrs->next; attr; attr = attr->next)
    {
      x_object_setattr(o, attr->key, attr->val);
      if (EQ(attr->key,X_STRING("bitrate")))
        {
          x_codec->cfg.rc_target_bitrate = atoi(attr->val);
        }
      else if (EQ(attr->key,X_STRING("width")))
        {
          x_codec->cfg.g_w = atoi(attr->val);
          x_codec->regs.cr0 |= xVPX_HAVE_W;
        }
      else if (EQ(attr->key,X_STRING("height")))
        {
          x_codec->cfg.g_h = atoi(attr->val);
          x_codec->regs.cr0 |= xVPX_HAVE_H;
        }
      else if (EQ(attr->key,X_STRING("threadnum")))
        {
          x_codec->cfg.g_threads = atoi(attr->val);
        }
      else if (EQ(attr->key,X_STRING("kf_max")))
        {
          x_codec->cfg.kf_max_dist = atoi(attr->val);
        }
      else if (EQ(attr->key,X_STRING("kf_min")))
        {
          x_codec->cfg.kf_min_dist = atoi(attr->val);
        }
      else if (EQ(attr->key,X_STRING("kf_mode")))
        {
          x_codec->cfg.kf_mode = atoi(attr->val);
        }
    }

  if (xVPX_IS_CONFIGURED(x_codec->regs.cr0)
      && !xVPX_IS_INITIALIZED(x_codec->regs.cr0))
    {
      if (vpx_codec_enc_init(&x_codec->vpx_codec, (vpx_codec_vp8_cx()), &x_codec->cfg, 0))
        {
          TRACE("Failed to init config: %s\n", vpx_codec_error(&x_codec->vpx_codec));
        }
      else
        {
          x_codec->regs.cr0 |= xVPX_INITIALIZED;
        }
    }
  else if (xVPX_IS_INITIALIZED(x_codec->regs.cr0))
    {
      x_memset(&x_codec->vpx_codec, 0, sizeof(x_codec->vpx_codec));
      if (vpx_codec_enc_config_set(&x_codec->vpx_codec, &x_codec->cfg))
        {
          TRACE("Failed to init config: %s\n", vpx_codec_error(&x_codec->vpx_codec));
        }
    }

  EXIT;
  return o;
}

static void
x_vpx_exit(x_object *o)
{
  ENTER;
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
  EXIT;
}

static void
x_vpx_on_create(x_object *o)
{
  int res;
  // vpx_codec_enc_cfg_t cfg;
  x_vpx_object_t *x_codec = (x_vpx_object_t *) o;
  ENTER;

  x_memset(&x_codec->cfg, 0, sizeof(x_codec->cfg));
#if 0
  res = vpx_codec_enc_config_default((vpx_codec_vp8_cx()), &x_codec->cfg, 0);
  if (res)
    {
      printf("Failed to get config: %s\n", vpx_codec_err_to_string(res));
      return -EBADFD;
    }
#endif

  x_codec->cfg.g_profile = 0;
  x_codec->cfg.kf_max_dist = 30;
  //x_codec->cfg.kf_min_dist = 0;
  x_codec->cfg.g_threads = 4;
  x_codec->cfg.g_pass = VPX_RC_ONE_PASS;
  x_codec->cfg.rc_end_usage = VPX_CBR;
  if (x_codec->cfg.rc_end_usage == VPX_CBR)
    {
      x_codec->cfg.rc_buf_initial_sz = 2000;
      x_codec->cfg.rc_buf_optimal_sz = 2000;
      x_codec->cfg.rc_buf_sz = 3000;
    }
  EXIT;
}

int
x_vpx_writev(x_object *o, const struct iovec *iov, int iovcnt,
    x_obj_attr_t *attr)
{
  ENTER;

  EXIT;
  return iovcnt;
}

static struct xobjectclass x_vpx_class;

static void
__vpx_on_x_path_event(x_object *obj)
{
  x_object *tmp;
  x_object *_o = _NEW ("vp8codec","gobee");
  ENTER;

  _ASET(_o,"description", "VP8 Video codec");
  _ASET(_o,"name", "vp8");

  tmp = x_object_add_path(X_OBJECT(obj), "__proc/video", NULL);
  _INS(tmp, X_OBJECT(_o));

  EXIT;
}

static x_path_listener_t vpx_bus_listener;

__x_plugin_visibility
__plugin_init void
x_vpx_init(void)
{
  ENTER;

  x_vpx_class.cname = "vp8codec";
  x_vpx_class.psize = sizeof(x_vpx_object_t) - sizeof(x_object);
  x_vpx_class.on_create = &x_vpx_on_create;
  x_vpx_class.match = &x_vpx_equals;
  x_vpx_class.on_assign = &x_vpx_on_assign;
  x_vpx_class.on_append = &x_vpx_on_append;
  x_vpx_class.finalize = &x_vpx_exit;
  x_vpx_class.try_writev = &x_vpx_writev;

  x_class_register(x_vpx_class.cname, &x_vpx_class);

  vpx_bus_listener.on_x_path_event = (void
  (*)(void*)) &__vpx_on_x_path_event;

  x_path_listener_add("/",&vpx_bus_listener);

  EXIT;
}

PLUGIN_INIT(x_vpx_init);

