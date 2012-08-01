/*
 * x_theora.c
 *
 *  Created on: Oct 26, 2011
 *      Author: artemka
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "mynet.h"
#include "x_theora.h"

static x_theora_codec_t cdc_ctx;
static x_theora_codec_t *cdc_ctx_p = &cdc_ctx;

static th_ycbcr_buffer ycbcr;

int
Render(const th_ycbcr_buffer *yuv);
static void
render_yuv_frame(th_ycbcr_buffer *yuv)
{
  Render((const th_ycbcr_buffer *) yuv);
}

#if 0
static unsigned char *pixels;

static unsigned char
clamp(int d)
  {
    if (d < 0)
    return 0;

    if (d > 255)
    return 255;

    return (unsigned char) d;
  }

static __inline__ void
chk_pixels(unsigned char *pxls, int siz)
  {
    if (pxls)
    return;

#if 0
    pxls = malloc(cursiz);
#else
    posix_memalign((void **) &pxls, sizeof(void *), siz);
#endif
    return;
  }

static void
yuv2rgb(th_ycbcr_buffer *ycbcr, unsigned char *rgb, int w, int h)
  {
    int i;
    int j;
    int Y;
    int u;
    int v;

    chk_pixels(pixels, w * h * 3);

    for (j = 0; j < h; j++)
      {
        for (i = 0; i < w; i++)
          {
            Y = (*ycbcr)[0].data[j * (*ycbcr)[0].stride + i];
            // if (!(i % 2) && !(j % 2))
              {
                u = (*ycbcr)[1].data[(j >> 1) * (*ycbcr)[1].stride + (i >> 1)];
                v = (*ycbcr)[2].data[(j >> 1) * (*ycbcr)[2].stride + (i >> 1)];
              }
            /*
             *         R = clip( (298 * (Y - 16)                   + 409 * (V - 128) + 128 ) >> 8 )
             *         G = clip( (298 * (Y - 16) - 100 * (U - 128) - 208 * (V - 128) + 128 ) >> 8 )
             *         B = clip( (298 * (Y - 16) + 516 * (U - 128)                   + 128 ) >> 8 )
             */
#if 0
            rgb[(j * w + i) * 3 + 0] = clamp(Y + 1.403 * (v - 128));
            rgb[(j * w + i) * 3 + 1] = clamp(Y - 0.344 * (u - 128) - 0.714 * (v - 128));
            rgb[(j * w + i) * 3 + 2] = clamp(Y + 1.773 * (u -128));
#else
            rgb[(j * w + i) * 3 + 0] = clamp((298 * (Y - 16) + 409 * (v - 128)
                    + 128) >> 8);
            rgb[(j * w + i) * 3 + 1] = clamp((298 * (Y - 16) - 100 * (u - 128)
                    - 208 * (v - 128) + 128) >> 8);
            rgb[(j * w + i) * 3 + 2] = clamp((298 * (Y - 16) + 516 * (u - 128)
                    + 128) >> 8);
#endif

          }
      }
  }
#endif

static x_theora_codec_t *
get_config_by_ident(unsigned int ident)
{
  x_theora_codec_t *cdc = cdc_ctx_p;
  return cdc;
}

void
x_theora_init(x_theora_codec_t *cdc)
{
  th_info_init(&cdc->tinfo);
  th_comment_init(&cdc->tcmnt);
}

static void
x_theora_dispatch_header_packet(const theora_packed_header_t *pconf)
{
  ogg_packet op;
  unsigned int hnum;
  long l1;
  long l2;
  char *hdata;
  x_theora_codec_t *cdc;

  printf("OK! Taken packed headers\n");
  hdata = (char *) &((const char *) pconf)[6];
  hnum = ntohl(((int *) pconf->data)[0]);
  l1 = ntohl(((int *) pconf->data)[1]);
  l2 = ntohl(((int *) pconf->data)[2]);
  printf("ident 0x%x\n", ntohl(pconf->ident));
  printf("plen %d\n", ntohs(pconf->plen));

  cdc = get_config_by_ident(pconf->ident);
  x_theora_init(cdc);

  /* start decoding */
  op.bytes = l1;
  op.packet = (unsigned char *) &pconf->data[12];
  op.b_o_s = 1;
  op.packetno = 1;
  printf("op.bytes=%ld\n", op.bytes);
  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  op.bytes = l2;
  op.packet = (unsigned char *) &pconf->data[12 + l1];
  op.b_o_s = 0;
  op.packetno++;
  printf("op.bytes=%ld\n", op.bytes);
  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  op.bytes = pconf->plen - l2 - l1;
  op.packet = (unsigned char *) &pconf->data[12 + l1 + l2];
  op.packetno++;
  printf("op.bytes=%ld\n", op.bytes);
  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  cdc->tctx = th_decode_alloc(&cdc->tinfo, cdc->tsinfo);

#if 1
  printf("VENDOR=%s\n", cdc->tcmnt.vendor);
  printf("version_major(%d)\n", cdc->tinfo.version_major);
  printf("version_minor(%d)\n", cdc->tinfo.version_minor);
  printf("version_subminor(%d)\n", cdc->tinfo.version_subminor);
  printf("frame_width(%d)\n", cdc->tinfo.frame_width);
  printf("frame_height(%d)\n", cdc->tinfo.frame_height);
  printf("fps_numerator(%d)\n", cdc->tinfo.fps_numerator);
  printf("fps_denominator(%d)\n", cdc->tinfo.fps_denominator);
  printf("aspect_numerator(%d)\n", cdc->tinfo.aspect_numerator);
  printf("aspect_denominator(%d)\n", cdc->tinfo.aspect_denominator);
  printf("target_bitrate(%d)\n", cdc->tinfo.target_bitrate);
  printf("quality(%d)\n", cdc->tinfo.quality);
#endif
}

static void
x_theora_dispatch_frame_packet(const theora_packed_header_t *pconf, int mark,
    int _seqno)
{
  static char *buffer = NULL;
  static int bufsiz = 0;
  static int curpos = 0;
  static int packetno = 1;
  int err;
  ogg_packet op;
  x_theora_codec_t *cdc;

  printf("Packet received: mark(%d), len(%d)\n",mark,ntohs(pconf->plen));

  switch (mark)
    {

  case 1:
    if (buffer)
      {
        free(buffer);
        buffer = NULL;
      }
    bufsiz = 0;
    curpos = 0;

  case 2:
    buffer = realloc(buffer, bufsiz + ntohs(pconf->plen));
    bufsiz += ntohs(pconf->plen);
    memcpy(&buffer[curpos], &pconf->data[0], ntohs(pconf->plen));
    curpos += ntohs(pconf->plen);
    return;

  case 0:
    if (buffer)
      {
        free(buffer);
        buffer = NULL;
      }
    bufsiz = 0;
    curpos = 0;
  case 3:
    buffer = realloc(buffer, bufsiz + ntohs(pconf->plen));
    bufsiz += ntohs(pconf->plen);
    memcpy(&buffer[curpos], &pconf->data[0], ntohs(pconf->plen));
    curpos += ntohs(pconf->plen);
    break;
    };

  cdc = get_config_by_ident(pconf->ident);

  op.bytes = (long)curpos;
  op.packet = (unsigned char *)&buffer[0];
  op.packetno = packetno++;
  op.b_o_s = 0;
  op.granulepos = 0;

  th_decode_packetin(cdc->tctx, &op, NULL);
  err = th_decode_ycbcr_out(cdc->tctx, ycbcr);

  if (err)
    {
      printf("ERROR decoding packet data\n");
    }

  render_yuv_frame(&ycbcr);
}

static void
__on_theora_packet_received(void *data, int len, void *cb_data)
{
  int tdt;
  rtp_hdr_t *rtp = (rtp_hdr_t *) data;
  int mark;

  tdt = (int) ((rtp->payload[3] >> 4) & 0x3);
  mark = (int) ((rtp->payload[3] >> 6) & 0x3);

  switch (tdt)
    {
  /* configuration header */
  case 1:
    {
      printf("OK! Taken packed headers\n");
      x_theora_dispatch_header_packet((theora_packed_header_t *) rtp->payload);
    }
    break;

    /* comment header */
  case 2:
    break;

    /* video frame packet */
  case 0:
    {
      x_theora_dispatch_frame_packet((theora_packed_header_t *) rtp->payload,
          mark, ntohl(rtp->seq));
    }
    break;

  default:
    break;
    };
}


void
on_theora_packet_received(void *data, int len, void *cb_data)
{
  rtp_hdr_t *rtp = (rtp_hdr_t *) data;

  switch (rtp->octet2.control2.pt)
  {
  case VP8_PTYPE:
    break;
  case THEORA_PTYPE:
    __on_theora_packet_received(data,len,cb_data);
    break;
  default:
    break;
  };
}
