/**
 * x_vpx.c
 *
 *  Created on: Feb 12, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[x_vpx] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>

#include <xmppagent.h>

#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

/* #define VPX_CODEC_DISABLE_COMPAT 1 */
#include <vpx/vpx_encoder.h>
#include <vpx/vp8cx.h>

#include "mynet.h"
#include "x_vp8.h"

#ifdef WIN32 
#	define _P(x) _##x
#else
#	define _P(x) x
#endif

typedef void
(*on_frame_ready)(vpx_image_t *);
typedef void
(*on_packet_ready)(vpx_codec_cx_pkt_t *);

static int seqno = 0;

static void
die_codec(vpx_codec_ctx_t *ctx, const char *s)
{
  const char *detail = vpx_codec_error_detail(ctx);
  printf("%s: %s\n", s, vpx_codec_error(ctx));
  if (detail)
    printf("    %s\n", detail);
  exit(EXIT_FAILURE);
}

#if 0
static int
x_vpx_decoder_decode(vpx_dec_ctx_t *_decoder, const char *buffer, size_t bufsiz)
  {
    vpx_image_t *img;
    unsigned char *buf;
    vpx_codec_iter_t iter = NULL;

    vpx_codec_decode(_decoder, buffer, bufsiz, 0, VPX_DL_REALTIME);
    while ((img = vpx_codec_get_frame(_decoder, &iter)))
      {
        printf("Decoded frame(fmt(%d)) bps=%d,%dx%d\n", img->fmt, img->bps,
            img->d_w, img->d_h);
        // img->img_data;
      }
    return 0;
  }
#endif

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

static int
x_vp8_write_frame(int ofd, vpx_image_t *img)
{
  int i;
  int j;
  int w = img->d_w;
  int h = img->d_h;
  int stride = img->stride[VPX_PLANE_Y];

  // write luminance
  for (j = 0; j < h; j++)
    {
      x_write(ofd, &img->planes[VPX_PLANE_Y][j * stride], w);
    }

  //write U plane
  stride = img->stride[VPX_PLANE_U];
  w = w >> 1;
  h = h >> 1;
  for (j = 0; j < h; j++)
    {
      x_write(ofd, &img->planes[VPX_PLANE_U][j * stride], w);
    }

  for (j = 0; j < h; j++)
    {
      x_write(ofd, &img->planes[VPX_PLANE_V][j * stride], w);
    }
  return 0;
}

int
__decode_vp8_pkt(vpx_codec_ctx_t *_decoder, const uint8_t *buffer,
    unsigned int bufsiz, int outfd)
{
  vpx_image_t *img;
  vpx_codec_iter_t iter = NULL;

  vpx_codec_decode(_decoder, buffer, bufsiz, 0, VPX_DL_REALTIME);
  while ((img = vpx_codec_get_frame(_decoder, &iter)))
    {
      // printf("Decoded frame(fmt(0x%x)) bps=%d,%dx%d\n", img->fmt, img->bps,
      //  img->d_w, img->d_h);
      x_vp8_write_frame(outfd, img);
    }
  return 0;
}

int
x_vpx_encoder_init(vpx_codec_ctx_t *_p_encoder, int numcores, int width,
    int height)
{
  int res;
  vpx_codec_enc_cfg_t cfg;

  res = vpx_codec_enc_config_default((vpx_codec_vp8_cx()), &cfg, 0);
  if (res)
    {
      printf("Failed to get config: %s\n", vpx_codec_err_to_string(res));
      return -EBADF;
    }

  /* Update the default configuration with our settings */
  printf("Initializing: %dx%d, BR=%d, cfg.g_timebase.den=%d\n", width, height,
      cfg.rc_target_bitrate, cfg.g_timebase.den);

  cfg.rc_target_bitrate = width * height * cfg.rc_target_bitrate / cfg.g_w
      / cfg.g_h * 2;
  cfg.g_w = width;
  cfg.g_h = height;
  cfg.g_profile = 0;
  //cfg.kf_mode = VPX_KF_AUTO;
  cfg.kf_max_dist = 30;
  //cfg.kf_min_dist = 0;
  cfg.g_threads = 4;
  cfg.g_pass = VPX_RC_ONE_PASS;
  cfg.rc_end_usage = VPX_CBR;
  if (cfg.rc_end_usage == VPX_CBR)
    {
      cfg.rc_buf_initial_sz = 2000;
      cfg.rc_buf_optimal_sz = 2000;
      cfg.rc_buf_sz = 3000;
    }

  //  cfg.g_timebase.num = 1001;
  //  cfg.g_timebase.den = 30000;

  if (vpx_codec_enc_init(_p_encoder, (vpx_codec_vp8_cx()), &cfg, 0))
    {
      printf("Failed to init config: %s\n", vpx_codec_error(_p_encoder));
      die_codec(_p_encoder, "vpx_codec_enc_init()");
      return -ENOMEM;
    }

  return 0;
}

#define MTU 1492

static int
x_vp8_rtp_send(int fd, const void *buf, int count, struct sockaddr *saddr,
    int addrlen, unsigned int t_stamp)
{
  unsigned char vp8hdr_buf[256];
  int hdroff = 0;
  rtp_hdr_t *_rtp;
  int len = sizeof(rtp_hdr_t);
  struct vp8_descriptor descriptor;
  int pcnt;
  int remainder = count;
  int bufoff = 0;
  int capacity = 0;
  int __rtp_mtu = (MTU - sizeof(rtp_hdr_t));
  int _count = count;

  _rtp = (rtp_hdr_t *) alloca(MTU);
  _rtp->octet1.control1.v = 2;
  _rtp->octet1.control1.p = 0;
  _rtp->octet1.control1.x = 0;
  _rtp->octet1.control1.cc = 0;
  _rtp->ssrc_id = htonl(0x11223344);
  _rtp->octet2.control2.pt = (VP8_PTYPE & 0x7f);

  // pack first header
  descriptor.frame_beginning = 1;
  descriptor.non_reference_frame = 0;
  descriptor.picture_id = (seqno & ((1 << 7) - 1));
  descriptor.fragmentation_info = NOT_FRAGMENTED;
  hdroff = pack_vp8_descriptor(&descriptor, &_rtp->payload[0], __rtp_mtu);

  // update length
  len = sizeof(rtp_hdr_t) + hdroff;
  capacity = (MTU - len);
  remainder = ((_count - capacity) > 0) ? (count - capacity) : 0;

  do
    {
      //printf("< l:%d;c:%d;r:%d;_c:%d;b:%d;h:%d;\n",
      //	len,capacity,remainder,_count,bufoff,hdroff);

      // calc remainder
      if (remainder)
        {
          if (!bufoff)
            {
              //printf("f\n");
              // set fragmentation info of first packet
              _rtp->payload[0] &= ~(0x3 << 1);
              _rtp->payload[0] |= ((FIRST_FRAGMENT & 0x3) << 1);
              descriptor.fragmentation_info = FIRST_FRAGMENT;
            }
          else
            {
              // set fragmentation info of middle packet
              descriptor.frame_beginning = 0;
              descriptor.non_reference_frame = 0;
              descriptor.picture_id = __UINT32_MAX__;
              descriptor.fragmentation_info = MIDDLE_FRAGMENT;
              hdroff = pack_vp8_descriptor(&descriptor, &_rtp->payload[0],
                  __rtp_mtu);
              //printf("m, h:%d\n",hdroff);
            }
        }
      else if (bufoff)
        {
          //printf("l\n");
          // set fragmentation info of last packet
          descriptor.frame_beginning = 0;
          descriptor.non_reference_frame = 0;
          descriptor.picture_id = __UINT32_MAX__;
          descriptor.fragmentation_info = LAST_FRAGMENT;
          hdroff = pack_vp8_descriptor(&descriptor, &_rtp->payload[0],
              __rtp_mtu);
        }
      else
        {
          //printf("0\n");
        }

      // update counters
      len = sizeof(rtp_hdr_t) + hdroff;
      capacity = (MTU - len);
      remainder = ((_count - capacity) > 0) ? (_count - capacity) : 0;
      len += _count - remainder;

      if ((descriptor.fragmentation_info == LAST_FRAGMENT)
          || !descriptor.fragmentation_info)
        _rtp->octet2.control2.m = 1;
      else
        _rtp->octet2.control2.m = 0;

      _rtp->seq = htons(seqno++);
      _rtp->t_stamp = htonl(t_stamp);

      memcpy(&_rtp->payload[hdroff], &((const char *) buf)[bufoff], _count
          - remainder);
      bufoff += _count - remainder;
      _count = remainder;
      remainder = ((_count - capacity) > 0) ? (_count - capacity) : 0;

      len = sendto(fd, (const char *) _rtp, len, 0, saddr, addrlen);
      // printf("sent %d bytes\n",len);
    }
  while ((_count));

  // printf("[rtp] seqno=%d, sent %d bytes\n", seqno, len);
  return len;
}

void
x_vpx_enc_run(unsigned char *pixels, int w, int h, int sock,
    struct sockaddr_in *cli)
{
  size_t len = 0;
  int p1x = 0;
  int p1y = 0;
  void *dat;
  vpx_codec_ctx_t _encoder;
  vpx_dec_ctx_t _decoder;
  vpx_image_t raw_yuv;
  const vpx_codec_cx_pkt_t *pkt;
  int frame_cnt = 0;
  int flags = 0;
  vpx_codec_iter_t iter;
  int outfd;
  int infd;
  int mem_fdmap;

  memset(&_encoder, 0, sizeof(_encoder));
  x_vpx_encoder_init(&_encoder, 1, w, h);

  memset(&_decoder, 0, sizeof(_decoder));
  x_vpx_decoder_init(&_decoder, 1);

  vpx_img_alloc(&raw_yuv, VPX_IMG_FMT_YV12, w, h, 0);
  memset(raw_yuv.planes[VPX_PLANE_Y], 0, raw_yuv.stride[VPX_PLANE_Y]
      * raw_yuv.d_h / 2);

  //  outfd = _P(open)("x_vp8_out.yuv",_P(O_CREAT) | _P(O_RDWR));
  infd = _P(open)("x_camera_in.yuv", _P(O_RDONLY));

  for (;;)
    {
#if 0
      raw_yuv.planes[VPX_PLANE_Y][p1y * w + p1x] = 0x0;
      p1x++;
      if (p1x > w)
        {
          p1x = 0;
          p1y++;
          if (p1y > h)
          p1y = 0;
        }
      raw_yuv.planes[VPX_PLANE_Y][p1y * w + p1x] = 0xff;
#endif
      //		int rbytes;
      //		int bufsiz = ycbcr[0].width*ycbcr[0].height
      //			+ ycbcr[1].width*ycbcr[1].height + ycbcr[2].width*ycbcr[2].height;

      /* read luminance */
      x_read(infd, raw_yuv.planes[VPX_PLANE_Y], raw_yuv.w * raw_yuv.h);
      /* read chroma U */
      x_read(infd, raw_yuv.planes[VPX_PLANE_U], (raw_yuv.w * raw_yuv.h) >> 2);
      /* read chroma V */
      x_read(infd, raw_yuv.planes[VPX_PLANE_V], (raw_yuv.w * raw_yuv.h) >> 2);

      //	x_vp8_write_frame(outfd,&raw_yuv);

      if (vpx_codec_encode(&_encoder, &raw_yuv, frame_cnt, 1, flags,
          VPX_DL_REALTIME ))
        {
          die_codec(&_encoder, "vpx_codec_enc_init()");
        }
      iter = NULL;
      while ((pkt = vpx_codec_get_cx_data(&_encoder, &iter)))
        {
          switch (pkt->kind)
            {
          case VPX_CODEC_CX_FRAME_PKT:
            // __decode_vp8_pkt(&_decoder, pkt->data.frame.buf, pkt->data.frame.sz, outfd);
            x_vp8_rtp_send((int) sock, (void *) pkt->data.frame.buf,
                (int) pkt->data.frame.sz, (struct sockaddr *) cli,
                (int) sizeof(*cli), seqno * 90);

            break;
          default:
            break;
            }
          x_write(1, (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
              && (pkt->data.frame.flags & VPX_FRAME_IS_KEY) ? "K" : (pkt->kind
              == VPX_CODEC_CX_FRAME_PKT) && (pkt->data.frame.flags
              & VPX_FRAME_IS_INVISIBLE) ? "I" : (pkt->kind
              == VPX_CODEC_CX_FRAME_PKT) && (pkt->data.frame.flags
              & VPX_FRAME_IS_DROPPABLE) ? "D" : (pkt->kind
              == VPX_CODEC_CX_FRAME_PKT) && (pkt->data.frame.flags & 0x3) ? "B"
              : ".", 1);
#ifndef WIN32
          fsync(1);
#endif
        }
      frame_cnt++;
#ifdef WIN32
      Sleep(20);
#else
      usleep(56000);
#endif
    }
}

