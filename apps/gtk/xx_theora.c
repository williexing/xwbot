#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <theora/theora.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

#define _BSD_SOURCE
#include <alloca.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include <unistd.h>

#include "mynet.h"
#include "x_theora.h"

static th_info tinfo;
static th_enc_ctx *tctx;
static th_comment tcmnt;
static theora_packed_header_t *pconf = NULL; // header packet
static int seqno = 121;
static volatile th_ycbcr_buffer s_ycbcr;
unsigned char *chromaBuf = NULL;

static void
__gobee_yuv_set_data(th_ycbcr_buffer *ycbcr, void *data)
{

  fprintf(stderr, "[YUV/JNI] %s() ENTER, chromaBuf=%p\n", __FUNCTION__,
      chromaBuf);

  (*ycbcr)[0].data = data;
  (*ycbcr)[1].data = &(*ycbcr)[0].data[0]
      + (*ycbcr)[0].width * (*ycbcr)[0].height;
  (*ycbcr)[2].data = &(*ycbcr)[1].data[0]
      + (*ycbcr)[1].width * (*ycbcr)[1].height;

//  chromaBuf[1024] = 0xff;
//  chromaBuf[620] = 0xff;
//  chromaBuf[621] = 0xff;
//  chromaBuf[622] = 0xff;
//
//  memset(&chromaBuf[0], 0xff, (*ycbcr)[0].width * (*ycbcr)[0].height * 3 / 2);

//  comvert nv21 to yuv planar
//  nv21_to_i420p(&data[(*ycbcr)[0].width * (*ycbcr)[0].height], chromaBuf,
//      (*ycbcr)[0].width, (*ycbcr)[0].height);

  fprintf(stderr, "[YUV/JNI] %s() EXIT\n", __FUNCTION__);
}

static void
__gobee_yuv_resize(th_ycbcr_buffer *ycbcr, int w, int h)
{
  /* submit data to encoder */
  (*ycbcr)[0].width = w; // (w + 15) & ~15;
  (*ycbcr)[0].height = h; //(h + 15) & ~15;
  (*ycbcr)[0].stride = (*ycbcr)[0].width;

//  chromaBuf = realloc(chromaBuf, (w * h) * 3 / 2 + 1);

  (*ycbcr)[1].width = (*ycbcr)[0].width >> 1;
  (*ycbcr)[1].height = (*ycbcr)[0].height >> 1;
  (*ycbcr)[1].stride = (*ycbcr)[1].width;

  (*ycbcr)[2].width = (*ycbcr)[1].width;
  (*ycbcr)[2].height = (*ycbcr)[1].height;
  (*ycbcr)[2].stride = (*ycbcr)[1].stride;

}

static void
__gobee_theora_send_headers(int sock, struct sockaddr *addr, int addrlen,
    int stamp)
{
  size_t len = 0;
  ogg_packet op;

  th_encode_flushheader(tctx, &tcmnt, &op);

  pconf = (theora_packed_header_t*) realloc(pconf,
      sizeof(theora_packed_header_t) + 12 + op.bytes);

  memcpy(&pconf->data[12], op.packet, op.bytes);
  pconf->plen = op.bytes;
  ((int *) pconf->data)[0] = htonl(3);
  ((int *) pconf->data)[1] = htonl(op.bytes);

  th_encode_flushheader(tctx, &tcmnt, &op);
  //printf("op.bytes=%d\n", op.bytes);
  pconf = (theora_packed_header_t*) realloc(pconf,
      sizeof(theora_packed_header_t) + 12 + pconf->plen + op.bytes);

  memcpy(&pconf->data[12 + pconf->plen], op.packet, op.bytes);
  pconf->plen += op.bytes;
  ((int *) pconf->data)[2] = htonl(op.bytes);

  th_encode_flushheader(tctx, &tcmnt, &op);
  //printf("op.bytes=%d\n", op.bytes);
  pconf = (theora_packed_header_t*) realloc(pconf,
      sizeof(theora_packed_header_t) + 12 + pconf->plen + op.bytes);
  memcpy(&pconf->data[12 + pconf->plen], op.packet, op.bytes);
  pconf->plen += op.bytes;

  len = sizeof(*pconf) + 12 + pconf->plen;
  pconf->plen = htons(pconf->plen);
  pconf->ident = htonl((1 << 4) | 0xffff1100);

  rtp_send(sock, (void *) pconf, (int) len, addr, addrlen, stamp, THEORA_PTYPE,
      seqno++);

}

void
gobee_theora_resize(int w, int h, int _stride, int sock, struct sockaddr *addr,
    int addrlen)
{
  time_t stamp = time(NULL);

  th_info_init(&tinfo);
  th_comment_init(&tcmnt);

  tinfo.frame_width = w;
  tinfo.frame_height = h;
  tinfo.pic_width = w;
  tinfo.pic_height = h;
  tinfo.pic_y = 0;
  tinfo.pic_x = 0;
  tinfo.fps_numerator = 30;
  tinfo.fps_denominator = 1;
  tinfo.aspect_denominator = 1;
  tinfo.aspect_numerator = 1;
  tinfo.target_bitrate = 200000;
  tinfo.quality = 12;
  tinfo.colorspace = TH_CS_ITU_REC_470BG; //TH_CS_UNSPECIFIED;
  tinfo.pixel_fmt = TH_PF_420;
  tinfo.keyframe_granule_shift = 100;
  tcmnt.vendor = "qqq";

  // recreate encoder
  if (tctx)
    th_encode_free(tctx);
  tctx = th_encode_alloc(&tinfo);

  __gobee_yuv_resize(&s_ycbcr, w, h);
  printf("sending...\n");
  __gobee_theora_send_headers(sock, addr, addrlen, stamp);
  printf("sent...\n");

}

#define PATH_MTU_RTP (1500 - sizeof(rtp_hdr_t))
#define PATH_MTU_THEORA (1500 - sizeof(rtp_hdr_t) - sizeof(theora_packed_header_t))

static int
gobee_theora_send_fragmented_rtp(ogg_packet *op, int sock,
    struct sockaddr *addr, int addrlen, int stamp)
{
  int elapsed = 0;
  int len = 0, _len;
  int sent = 0;
  len = (int) op->bytes;
  static char packbuf[PATH_MTU_RTP];
  theora_packed_header_t *dataout = (theora_packed_header_t *) packbuf;
  int mark = 0;
  int tdt = 0;
  int numpackets = 1;

  fprintf(stderr, "Sending... len(%d), MTU(%d) bytes\n", len, PATH_MTU_THEORA);

  for (sent = 0, elapsed = len; elapsed > 0; sent += _len, elapsed -= _len)
    {
      _len = PATH_MTU_THEORA < elapsed ? PATH_MTU_THEORA : elapsed;

      fprintf(stderr, "packet: elapsed(%d), sent(%d),"
          " _len(%d), \n", elapsed, sent, _len);

      if (PATH_MTU_THEORA < elapsed)
        {
          if (sent)
            {
              mark = 2;
            }
          else
            {
              mark = 1;
            }
        }
      else if (sent)
        {
          mark = 3;
        }

      dataout->plen = htons((short) _len);
      dataout->ident = htonl(
          (0xffff1100 << 8) | ((mark & 0x3) << 6) | ((tdt & 0x3) << 4)
              | (numpackets & 0xf));

      memcpy(&dataout->data[0], &op->packet[sent], _len);

      rtp_send(sock, (void *) dataout,
          (int) sizeof(theora_packed_header_t) + _len, addr, addrlen, stamp,
          THEORA_PTYPE, seqno++);
    }

}

void
gobee_theora_push_frame(void *frame, int sock, struct sockaddr *addr,
    int addrlen)
{
  int err;
  ogg_packet op;
  time_t stamp = time(NULL);

  fprintf(stderr, "[YUV/JNI] %s() ENTER, framebuf=%p\n", __FUNCTION__, frame);

  __gobee_yuv_set_data(&s_ycbcr, frame);

  th_encode_ycbcr_in(tctx, &s_ycbcr);

  fprintf(stderr, "[YUV/JNI] %s() EXIT\n", __FUNCTION__);

  if ((err = th_encode_packetout(tctx, 0, &op)) > 0
  /* && op.bytes > 0*/)
    {
      gobee_theora_send_fragmented_rtp(&op, sock, addr, addrlen, stamp);
    }
  else
    {
      fprintf(stderr, "error encoding packet, %d\n", err);
    }
}
