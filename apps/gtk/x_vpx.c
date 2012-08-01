/**
 * x_vpx.c
 *
 *  Created on: Feb 12, 2012
 *      Author: artemka
 */

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#include <vpx/vpx_encoder.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8cx.h>
#include <vpx/vp8dx.h>

static int
x_vpx_decoder_decode(vpx_dec_ctx_t *_decoder, const char *buffer, size_t bufsiz)
{
  vpx_image_t *img;
  unsigned char *buf;
  vpx_codec_iter_t iter = NULL;

  vpx_codec_decode(_decoder, buffer, bufsiz, 0, VPX_DL_REALTIME);
  while ((img = vpx_codec_get_frame(_decoder, &iter)))
    {
      buf = img->img_data;
    }
  return 0;
}

int
x_vpx_decoder_init(vpx_dec_ctx_t *_decoder, int numcores)
{
  vpx_codec_dec_cfg_t cfg;
  vpx_codec_flags_t flags = 0;
  int err;

  cfg.threads = 1;
  cfg.h = cfg.w = 0; // set after decode

#if WEBRTC_LIBVPX_VERSION >= 971
  flags = VPX_CODEC_USE_ERROR_CONCEALMENT | VPX_CODEC_USE_POSTPROC;
#ifdef INDEPENDENT_PARTITIONS
  flags |= VPX_CODEC_USE_INPUT_PARTITION;
#endif
#endif

  if (vpx_codec_dec_init(_decoder, vpx_codec_vp8_dx(), &cfg, flags))
    {
      return -ENOMEM;
    }

#if WEBRTC_LIBVPX_VERSION >= 971
  vp8_postproc_cfg_t ppcfg;
  // Disable deblocking for now due to uninitialized memory being returned.
  ppcfg.post_proc_flag = 0;
  // Strength of deblocking filter. Valid range:[0,16]
  //ppcfg.deblocking_level = 3;
  vpx_codec_control(_decoder, VP8_SET_POSTPROC, &ppcfg);
#endif

  return 0;
}

