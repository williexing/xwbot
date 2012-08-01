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

#undef DEBUG_PRFX
#include <xwlib/x_config.h>
#if TRACE_XVIDEO_THEORA_ON
#define DEBUG_PRFX "[xTHEORA_API] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>
#include <crypt/base64.h>

#include "x_theora_api.h"

/**
 * @todo This should be replaced with a callback function
 * from xtheora_plugin or something similar...
 */
void
x_theora_render_yuv_frame(x_theora_context_t *ctx, th_ycbcr_buffer ycbcr);

void
x_theora_context_init(x_theora_context_t *cdc)
{
  char buf[64];
  int hoffset = 3;
  int w = 320;
  int h = 240;
  ogg_packet op;
  theora_packed_header_t *pconf = 0;
  char *confstr;
  unsigned char *hdata;
  unsigned char *confbuf = NULL;

  th_enc_ctx *ed = NULL;

  memset(cdc, 0, sizeof(x_theora_context_t));
  th_info_init(&cdc->tinfo);
  th_comment_init(&cdc->tcmnt);
  th_setup_free(cdc->tsinfo);
#if 0
  if (!cdc->header)
    {
      cdc->ident = 0x112233;
      cdc->tinfo.frame_width = w;
      cdc->tinfo.frame_height = h;
      cdc->tinfo.pic_width = w;
      cdc->tinfo.pic_height = h;
      cdc->tinfo.pic_y = 0;
      cdc->tinfo.pic_x = 0;
      cdc->tinfo.fps_numerator = 15;
      cdc->tinfo.fps_denominator = 1;
      cdc->tinfo.aspect_denominator = 1;
      cdc->tinfo.aspect_numerator = 1;
      cdc->tinfo.colorspace = TH_CS_ITU_REC_470BG; //TH_CS_UNSPECIFIED;
      cdc->tinfo.pixel_fmt = TH_PF_420;
      cdc->tinfo.keyframe_granule_shift = 100;
      cdc->tinfo.target_bitrate = 400000;
      cdc->tinfo.quality = 12;
      cdc->tcmnt.vendor = "qqq";

      ed = th_encode_alloc(&cdc->tinfo);

      memset(&op, 0, sizeof(ogg_packet));
      th_encode_flushheader(ed, &cdc->tcmnt, &op);

      TRACE("First header: len %d\n", op.bytes);

      pconf = (theora_packed_header_t*) x_malloc(
          THEORA_HDR_SIZE + hoffset + op.bytes);

      memcpy(&pconf->data[hoffset], op.packet, op.bytes);
      pconf->plen = op.bytes;
      pconf->data[0] = 2 & 0xff;
      pconf->data[1] = op.bytes & 0xff;

//      memset(&op, 0, sizeof(ogg_packet));
      th_encode_flushheader(ed, &cdc->tcmnt, &op);

      pconf = (theora_packed_header_t*) x_realloc(pconf,
          THEORA_HDR_SIZE + hoffset + pconf->plen + op.bytes);

      TRACE("Second header: len %d\n", op.bytes);

      memcpy(&pconf->data[hoffset + pconf->plen], op.packet, op.bytes);
      pconf->plen += op.bytes;
      pconf->data[2] = (op.bytes & 0xff);

//      memset(&op, 0, sizeof(ogg_packet));
      th_encode_flushheader(ed, &cdc->tcmnt, &op);
      pconf = (theora_packed_header_t*) x_realloc(pconf,
          sizeof(theora_packed_header_t) + hoffset + pconf->plen + op.bytes);
      memcpy(&pconf->data[hoffset + pconf->plen], op.packet, op.bytes);
      pconf->plen += op.bytes;

      TRACE("Third header: len %d\n", op.bytes);

      cdc->header = pconf;
      cdc->header_len = THEORA_HDR_SIZE + hoffset + pconf->plen;

      pconf->plen = htons(pconf->plen);
      pconf->ident = htonl((1 << 4) | (cdc->ident << 8));

      th_encode_free(ed);
    }

  confbuf = x_malloc(ntohs(cdc->header->plen) + 7 + hoffset + 20);
  hdata = confbuf;

  *((uint32_t *) hdata) = htonl(1); // numpkts
  hdata += 4;

  *((uint32_t *) hdata) = htonl(cdc->ident << 8);
  hdata += 3;

  x_memcpy(hdata, &cdc->header->plen, ntohs(cdc->header->plen) + hoffset);

  confstr = bin2hex(confbuf, ntohs(cdc->header->plen) + 7 + hoffset);

  if (confstr)
    {
//      _ASET(X_OBJECT(cdc), _XS("configuration"), confstr);

      /**
       * @todo FIXME this should be done in some other way
       */
      theora_read_str_config(cdc, confstr);
      x_free(confstr);
      x_free(confbuf);
    }
#endif
}

void
x_theora_dispatch_header_packet(x_theora_context_t *cdc,
    const theora_packed_header_t *pconf)
{
  ogg_packet op;
  unsigned int hnum;
  long l1;
  long l2;

  TRACE("Packed headers\n");

  hnum = ntohl(((uint32_t *) (void *) pconf->data)[0]);
  l1 = ntohl(((uint32_t *) (void *) pconf->data)[1]);
  l2 = ntohl(((uint32_t *) (void *) pconf->data)[2]);

  TRACE("ident 0x%x\n", ntohl(pconf->ident));
  TRACE("plen %d\n", ntohs(pconf->plen));

  /* start decoding */
  op.bytes = l1;
  op.packet = (unsigned char *) &pconf->data[12];
  op.b_o_s = 1;
  op.packetno = 1;

  TRACE("op.bytes=%ld\n", op.bytes);

  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  op.bytes = l2;
  op.packet = (unsigned char *) &pconf->data[12 + l1];
  op.b_o_s = 0;
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  op.bytes = pconf->plen - l2 - l1;
  op.packet = (unsigned char *) &pconf->data[12 + l1 + l2];
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  cdc->codec.dd = th_decode_alloc(&cdc->tinfo, cdc->tsinfo);

#if 1
  TRACE("VENDOR=%s\n", cdc->tcmnt.vendor);
  TRACE("version_major(%d)\n", cdc->tinfo.version_major);
  TRACE("version_minor(%d)\n", cdc->tinfo.version_minor);
  TRACE("version_subminor(%d)\n", cdc->tinfo.version_subminor);
  TRACE("frame_width(%d)\n", cdc->tinfo.frame_width);
  TRACE("frame_height(%d)\n", cdc->tinfo.frame_height);
  TRACE("fps_numerator(%d)\n", cdc->tinfo.fps_numerator);
  TRACE("fps_denominator(%d)\n", cdc->tinfo.fps_denominator);
  TRACE("aspect_numerator(%d)\n", cdc->tinfo.aspect_numerator);
  TRACE("aspect_denominator(%d)\n", cdc->tinfo.aspect_denominator);
  TRACE("target_bitrate(%d)\n", cdc->tinfo.target_bitrate);
  TRACE("quality(%d)\n", cdc->tinfo.quality);
#endif

}

#if 0
int
theora_write_str_config(x_theora_context_t *cdc, x_string *confstr)
  {
    ogg_packet op;
    unsigned char *hdata;
    unsigned char *ptr = NULL;
    uint32_t plen;
    uint32_t ident;
    uint32_t numpkts;
    int hlen;
    unsigned int hnum;
    long l1;
    long l2;
    int w;
    int h;

    int fd;

    ENTER;

    TRACE();

    memset((void *) &op, 0, sizeof(ogg_packet));

    hdata = (unsigned char *) hex2bin(confstr, &hlen);
    /* try to decide base64 */
    if (!hdata)
      {
        hdata = (unsigned char *) base64_decode(confstr, strlen(confstr), &hlen);
        TRACE("Decoding base64: hdata=0x%p, hlen(%d)\n", hdata, hlen);
      }

    ptr = hdata;
    fd = open("theora_hdr.dmp", O_CREAT | O_TRUNC | O_RDWR, S_IRWXG);
    write(fd, hdata, hlen);
    close(fd);

    numpkts = ntohl(*(uint32_t *) hdata);
    hdata += 4;

    cdc->ident = ntohl(*(uint32_t *) (hdata)) >> 8;
    hdata += 3;

    plen = (int) ntohs(*(uint16_t *) (hdata));
    hdata += 2;

    hnum = (uint32_t) *hdata++;
    l1 = (uint32_t) *hdata++;
    l2 = (uint32_t) *hdata++;

    TRACE("nupkts(%d),plen(%d),ident(0x%x)\n", numpkts, plen, ident);
    TRACE("hnum(%d),l1(%ld),l2(%ld)\n", hnum, l1, l2);

    /* start decoding */
    op.bytes = l1;
    op.packet = hdata;
    op.b_o_s = 1;
    op.packetno = 1;

    TRACE("op.bytes=%ld\n", op.bytes);

    th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

    op.bytes = l2;
    op.packet = hdata + l1;
    op.b_o_s = 0;
    op.packetno++;

    TRACE("op.bytes=%ld\n", op.bytes);

    th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

    op.bytes = plen - l2 - l1;
    op.packet = hdata + l1 + l2;
    op.packetno++;

    TRACE("op.bytes=%ld\n", op.bytes);

    th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

    if (TYPE_DECODER == cdc->type)
      {
//      cdc->tinfo.colorspace = TH_CS_ITU_REC_470BG; //TH_CS_UNSPECIFIED;
//      cdc->tinfo.pixel_fmt = TH_PF_444;
        cdc->codec.dd = th_decode_alloc(&cdc->tinfo, cdc->tsinfo);
      }
    else if (TYPE_ENCODER == cdc->type)
      {
//      cdc->tinfo.quality = 16;
//      cdc->tinfo.keyframe_granule_shift = 64;
//      cdc->tinfo.target_bitrate = 300000;
//       w = cdc->tinfo.frame_width;
//       h = cdc->tinfo.frame_height;
//
//       th_info_init(&cdc->tinfo);
//       th_comment_init(&cdc->tcmnt);
//       th_setup_free(cdc->tsinfo);
//
//       cdc->tinfo.frame_width = w;
//       cdc->tinfo.frame_height = h;
//       cdc->tinfo.pic_width = w;
//       cdc->tinfo.pic_height = h;
//       cdc->tinfo.pic_y = 0;
//       cdc->tinfo.pic_x = 0;
        cdc->tinfo.fps_numerator = 15;
//       cdc->tinfo.fps_denominator = 1;
//       cdc->tinfo.aspect_denominator = 1;
//       cdc->tinfo.aspect_numerator = 1;
//       cdc->tinfo.colorspace = TH_CS_ITU_REC_470BG; //TH_CS_UNSPECIFIED;
//       cdc->tinfo.pixel_fmt = TH_PF_420;
//       cdc->tinfo.keyframe_granule_shift = 1;
//       cdc->tinfo.target_bitrate = 400000;
//       cdc->tinfo.quality = 12;
//       cdc->tcmnt.vendor = "qqq";

        cdc->tinfo.keyframe_granule_shift = 100;
        cdc->codec.ed = th_encode_alloc(&cdc->tinfo);
      }

    if (ptr)
    free(ptr);

#if 1
    TRACE("%s:\n",
        cdc->type == TYPE_ENCODER? "ENCODER" : cdc->type == TYPE_DECODER? "DECODER" : "UNKNOWN TYPE");
    TRACE("VENDOR=%s\n", cdc->tcmnt.vendor);
    TRACE("version_major(%d)\n", cdc->tinfo.version_major);
    TRACE("version_minor(%d)\n", cdc->tinfo.version_minor);
    TRACE("version_subminor(%d)\n", cdc->tinfo.version_subminor);
    TRACE("frame_width(%d)\n", cdc->tinfo.frame_width);
    TRACE("frame_height(%d)\n", cdc->tinfo.frame_height);
    TRACE("pic_width(%d)\n", cdc->tinfo.pic_width);
    TRACE("pic_height(%d)\n", cdc->tinfo.pic_height);
    TRACE("fps_numerator(%d)\n", cdc->tinfo.fps_numerator);
    TRACE("fps_denominator(%d)\n", cdc->tinfo.fps_denominator);
    TRACE("aspect_numerator(%d)\n", cdc->tinfo.aspect_numerator);
    TRACE("aspect_denominator(%d)\n", cdc->tinfo.aspect_denominator);
    TRACE("target_bitrate(%d)\n", cdc->tinfo.target_bitrate);
    TRACE("pixel_format(%d)\n", cdc->tinfo.pixel_fmt);
    TRACE("ColorSpace(%d)\n", cdc->tinfo.colorspace);
    TRACE("quality(%d)\n", cdc->tinfo.quality);
#endif

    cdc->lastframe[0].width = cdc->tinfo.frame_width;
    cdc->lastframe[0].height = cdc->tinfo.frame_height;
    cdc->lastframe[0].stride = cdc->tinfo.frame_width;

    cdc->lastframe[1].width = cdc->tinfo.frame_width >> 1;
    cdc->lastframe[1].height = cdc->tinfo.frame_height >> 1;
    cdc->lastframe[1].stride = cdc->tinfo.frame_width >> 1;

    cdc->lastframe[2].width = cdc->tinfo.frame_width >> 1;
    cdc->lastframe[2].height = cdc->tinfo.frame_height >> 1;
    cdc->lastframe[2].stride = cdc->tinfo.frame_width >> 1;

    EXIT;
    return;

  }
#endif

void
theora_read_str_config(x_theora_context_t *cdc, const char *confstr)
{
  ogg_packet op;
  unsigned char *hdata;
  unsigned char *ptr = NULL;
  uint32_t plen;
  uint32_t ident;
  uint32_t numpkts;
  int hlen;
  unsigned int hnum;
  long l1;
  long l2;
  int w;
  int h;

  int fd;

  ENTER;

  TRACE();

  memset((void *) &op, 0, sizeof(ogg_packet));

  hdata = (unsigned char *) hex2bin(confstr, &hlen);
  /* try to decide base64 */
  if (!hdata)
    {
      hdata = (unsigned char *) base64_decode(confstr, strlen(confstr), &hlen);
      TRACE("Decoding base64: hdata=0x%p, hlen(%d)\n", hdata, hlen);
    }

  ptr = hdata;
  fd = open("theora_hdr.dmp", O_CREAT | O_TRUNC | O_RDWR, S_IRWXG);
  write(fd, hdata, hlen);
  close(fd);

  numpkts = ntohl(*(uint32_t *) hdata);
  hdata += 4;

  cdc->ident = ntohl(*(uint32_t *) (hdata)) >> 8;
  hdata += 3;

  plen = (int) ntohs(*(uint16_t *) (hdata));
  hdata += 2;

  hnum = (uint32_t) *hdata++;
  l1 = (uint32_t) *hdata++;
  l2 = (uint32_t) *hdata++;

  TRACE("nupkts(%d),plen(%d),ident(0x%x)\n", numpkts, plen, ident);
  TRACE("hnum(%d),l1(%ld),l2(%ld)\n", hnum, l1, l2);

  /* start decoding */
  op.bytes = l1;
  op.packet = hdata;
  op.b_o_s = 1;
  op.packetno = 1;

  TRACE("op.bytes=%ld\n", op.bytes);

  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  op.bytes = l2;
  op.packet = hdata + l1;
  op.b_o_s = 0;
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  op.bytes = plen - l2 - l1;
  op.packet = hdata + l1 + l2;
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);

  if (TYPE_DECODER == cdc->type)
    {
//      cdc->tinfo.colorspace = TH_CS_ITU_REC_470BG; //TH_CS_UNSPECIFIED;
//      cdc->tinfo.pixel_fmt = TH_PF_444;
      cdc->codec.dd = th_decode_alloc(&cdc->tinfo, cdc->tsinfo);
    }
  else if (TYPE_ENCODER == cdc->type)
    {
//      cdc->tinfo.quality = 16;
//      cdc->tinfo.keyframe_granule_shift = 64;
//      cdc->tinfo.target_bitrate = 300000;
//       w = cdc->tinfo.frame_width;
//       h = cdc->tinfo.frame_height;
//
//       th_info_init(&cdc->tinfo);
//       th_comment_init(&cdc->tcmnt);
//       th_setup_free(cdc->tsinfo);
//
//       cdc->tinfo.frame_width = w;
//       cdc->tinfo.frame_height = h;
//       cdc->tinfo.pic_width = w;
//       cdc->tinfo.pic_height = h;
//       cdc->tinfo.pic_y = 0;
//       cdc->tinfo.pic_x = 0;
//      cdc->tinfo.fps_numerator = 15;
//       cdc->tinfo.fps_denominator = 1;
//       cdc->tinfo.aspect_denominator = 1;
//       cdc->tinfo.aspect_numerator = 1;
//       cdc->tinfo.colorspace = TH_CS_ITU_REC_470BG; //TH_CS_UNSPECIFIED;
//       cdc->tinfo.pixel_fmt = TH_PF_420;
//       cdc->tinfo.keyframe_granule_shift = 64;
//       cdc->tinfo.target_bitrate = 400000;
//       cdc->tinfo.quality = 12;
//       cdc->tcmnt.vendor = "qqq";

//      cdc->tinfo.keyframe_granule_shift = 100;
      cdc->codec.ed = th_encode_alloc(&cdc->tinfo);
    }
  else
    {
      TRACE("Unknown mode type\n");
    }

  if (ptr)
    free(ptr);

#if 1
  TRACE("%s:\n",
      cdc->type == TYPE_ENCODER? "ENCODER" : cdc->type == TYPE_DECODER? "DECODER" : "UNKNOWN TYPE");
  TRACE("VENDOR=%s\n", cdc->tcmnt.vendor);
  TRACE("version_major(%d)\n", cdc->tinfo.version_major);
  TRACE("version_minor(%d)\n", cdc->tinfo.version_minor);
  TRACE("version_subminor(%d)\n", cdc->tinfo.version_subminor);
  TRACE("frame_width(%d)\n", cdc->tinfo.frame_width);
  TRACE("frame_height(%d)\n", cdc->tinfo.frame_height);
  TRACE("pic_width(%d)\n", cdc->tinfo.pic_width);
  TRACE("pic_height(%d)\n", cdc->tinfo.pic_height);
  TRACE("fps_numerator(%d)\n", cdc->tinfo.fps_numerator);
  TRACE("fps_denominator(%d)\n", cdc->tinfo.fps_denominator);
  TRACE("aspect_numerator(%d)\n", cdc->tinfo.aspect_numerator);
  TRACE("aspect_denominator(%d)\n", cdc->tinfo.aspect_denominator);
  TRACE("target_bitrate(%d)\n", cdc->tinfo.target_bitrate);
  TRACE("pixel_format(%d)\n", cdc->tinfo.pixel_fmt);
  TRACE("ColorSpace(%d)\n", cdc->tinfo.colorspace);
  TRACE("quality(%d)\n", cdc->tinfo.quality);
#endif

  cdc->lastframe[0].width = cdc->tinfo.frame_width;
  cdc->lastframe[0].height = cdc->tinfo.frame_height;
  cdc->lastframe[0].stride = cdc->tinfo.frame_width;

  cdc->lastframe[1].width = cdc->tinfo.frame_width >> 1;
  cdc->lastframe[1].height = cdc->tinfo.frame_height >> 1;
  cdc->lastframe[1].stride = cdc->tinfo.frame_width >> 1;

  cdc->lastframe[2].width = cdc->tinfo.frame_width >> 1;
  cdc->lastframe[2].height = cdc->tinfo.frame_height >> 1;
  cdc->lastframe[2].stride = cdc->tinfo.frame_width >> 1;

  EXIT;
  return;

}

static void
x_theora_dispatch_frame_packet(x_theora_context_t *cdc,
    const theora_packed_header_t *pconf, int totlen, int mark)
{
  static char *buffer = NULL;
  static int bufsiz = 0;
  static int curpos = 0;
  static ogg_int64_t packetno = 3; /* skip header packets */
  int plen;
  int err = -1;
  ogg_packet op;
  int numpacks = ((char *) (void *) pconf)[3] & 0xf;

  plen = ntohs(*(uint16_t *) &((char *) pconf)[4]);

  TRACE("Packet received: mark(%d), payloadlen(%d), "
  "packet=%p, data=%p, packetnum=%d, dataoffst=%d, totlen=%d\n",
      mark, plen, pconf, &((char *) pconf)[0], numpacks, (int)(&((char *) pconf)[6] - (char *)(void *)pconf), totlen);

  switch (mark)
    {

  case 1:
    curpos = 0;
    memset(&buffer[0], 0, bufsiz);

  case 2:
    if ((curpos + plen) > bufsiz)
      {
        bufsiz = curpos + plen;
        buffer = x_realloc(buffer, bufsiz);
      }
    memcpy(&buffer[curpos], &((char *) pconf)[6], plen);
    curpos += plen;
    return;

  case 0:
    curpos = 0;
  case 3:
    if ((curpos + plen) > bufsiz)
      {
        bufsiz = curpos + plen;
        buffer = x_realloc(buffer, bufsiz);
      }
    memcpy(&buffer[curpos], &((char *) pconf)[6], plen);
    curpos += plen;
    break;
    };

  op.bytes = (long) curpos;
  op.packet = (unsigned char *) &buffer[0];
//  op.packetno = packetno++;
  op.packetno = 0;
  op.b_o_s = 0;
  op.e_o_s = 0;
  op.granulepos = -1;

  th_decode_packetin(cdc->codec.dd, &op, &op.granulepos);
  err = th_decode_ycbcr_out(cdc->codec.dd, cdc->lastframe);
  /*

   if (err)
   TRACE("ERROR decoding packet data\n");
   else
   TRACE("Decode Ok, %p, bytes=%d\n", cdc->lastframe, curpos);
   */

  x_theora_render_yuv_frame(cdc, cdc->lastframe);
}

void
__on_theora_packet_received(x_theora_context_t *cdc, void *_data, int len)
{
  int tdt;
  int mark;
  const char *data = (const char *) _data;

  tdt = (int) ((data[3] >> 4) & 0x3);
  mark = (int) ((data[3] >> 6) & 0x3);

  TRACE("OK! Theora payload: len=%d\n", len);

  switch (tdt)
    {
  /* configuration header */
  case 1:
    {
      x_theora_dispatch_header_packet(cdc, (theora_packed_header_t *) data);
    }
    break;

    /* comment header */
  case 2:
    break;

    /* video frame packet */
  case 0:
    {
      x_theora_dispatch_frame_packet(cdc, (theora_packed_header_t *) data, len,
          mark);
    }
    break;

  default:
    break;
    };
}
