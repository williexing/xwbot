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
#include <x_config.h>
#if TRACE_XVIDEO_THEORA_ON
#define DEBUG_PRFX "[xTHEORA_API] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>
#include <crypt/base64.h>

#include "x_theora_api.h"


static unsigned char *
write32(unsigned char **data_, uint32_t val)
{
    unsigned char *data = *data_;
    data[0] = (val >> 24)& 0xff;
    data[1] = (val >> 16)& 0xff;
    data[2] = (val >> 8)& 0xff;
    data[3] = val& 0xff;
    *data_ += 4;
    return *data_;
}

static unsigned char *
write16(unsigned char **data_, uint32_t val)
{
    unsigned char *data = *data_;
    data[0] = (val >> 8)& 0xff;
    data[1] = val& 0xff;
    *data_ += 2;
    return *data_;
}

static uint32_t
read32(unsigned char **data_)
{
    uint32_t val = 0;
    unsigned char *data = *data_;
    val = ((data[0] & 0xff) << 24)
            | ((data[1] & 0xff) << 16)
            | ((data[2] & 0xff) << 8)
            | data[3] & 0xff;
    *data_ += 4;
    return val;
}

static uint32_t
read16(unsigned char **data_)
{
    uint32_t val = 0;
    unsigned char *data = *data_;
    val = ((data[0] & 0xff) << 8)
            | data[1] & 0xff;
    *data_ += 2;
    return val;
}


/**
 * @todo This should be replaced with a callback function
 * from xtheora_plugin or something similar...
 */
void
x_theora_render_yuv_frame(x_theora_context_t *ctx, th_ycbcr_buffer ycbcr);

static void
__x_theora_lastframe_update(x_theora_context_t *cdc)
{
    cdc->lastframe[0].width = cdc->tinfo.pic_width;
    cdc->lastframe[0].height = cdc->tinfo.frame_height;
    cdc->lastframe[0].stride = cdc->tinfo.frame_width;

    cdc->lastframe[1].width = cdc->tinfo.pic_width / 2;
    cdc->lastframe[1].height = cdc->tinfo.frame_height / 2;
    cdc->lastframe[1].stride = cdc->tinfo.frame_width / 2;

    cdc->lastframe[2].width = cdc->tinfo.pic_width / 2;
    cdc->lastframe[2].height = cdc->tinfo.frame_height / 2;
    cdc->lastframe[2].stride = cdc->tinfo.frame_width / 2;
}

void
x_theora_decoder_init(x_theora_context_t *cdc)
{
    th_dec_ctx *ddtmp;
#if 1
    TRACE("DECODER:\n");
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
    ddtmp = cdc->codec.dd;
    cdc->codec.dd = th_decode_alloc(&cdc->tinfo, cdc->tsinfo);
    if (ddtmp)
    {
        th_decode_free(ddtmp);
    }
    th_setup_free(cdc->tsinfo);
    cdc->tsinfo = NULL;
    TRACE("Decoder(%p)\n",cdc->codec.dd);
}

void
x_theora_encoder_init(x_object *obj, x_theora_context_t *cdc)
{
    //  x_object *param;
#ifdef DEBUG
    int fd;
#endif
    int err;
    ogg_packet op;
    char *ptr;
    char *confstr;
    unsigned char *hdata;
    theora_packed_header_t *pconf = 0;
    unsigned char *confbuf = NULL;
    int hoffset = 3;
    th_enc_ctx *ed = NULL;
    th_enc_ctx *edtmp = NULL;
    int w = -1;
    int h = -1;
    int s = -1;

    const char *wstr;
    const char *hstr;
    const char *sstr;

#if 1

    wstr = _AGET(obj,"width");
    hstr = _AGET(obj,"height");
    sstr = _AGET(obj,"stride");

    if (!(wstr && hstr))
    {
        BUG();
        return;
    }

    if(!sstr)
    {
        sstr = wstr; // yuv only
    }

    w = atoi(wstr);
    h = atoi(hstr);
    s = atoi(sstr);

    TRACE("Creating enc: %dx%dx%d\n", w,h,s);

//    if (!cdc->header)
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
        cdc->tinfo.keyframe_granule_shift = 200;
        cdc->tinfo.target_bitrate = 300000;
        cdc->tinfo.quality = 0;
        cdc->tcmnt.vendor = "qqq";

        ed = th_encode_alloc(&cdc->tinfo);

//        x_memset(&op, 0, sizeof(ogg_packet));
        err = th_encode_flushheader(ed, &cdc->tcmnt, &op);

        TRACE("First header: ed(%p) len %d, err=%d\n", ed, op.bytes, err);

        pconf = (theora_packed_header_t*) x_malloc(
                    sizeof(theora_packed_header_t) + hoffset + op.bytes);

        ptr = (void *)pconf + sizeof(theora_packed_header_t);
        x_memcpy(ptr + hoffset, op.packet, op.bytes);
        pconf->plen = (uint16_t)op.bytes;

        ptr[0] = 2 & 0xff;
        ptr[1] = op.bytes & 0xff;

        //      memset(&op, 0, sizeof(ogg_packet));
        err = th_encode_flushheader(ed, &cdc->tcmnt, &op);

        pconf = (theora_packed_header_t*) x_realloc(pconf,
                                                    sizeof(theora_packed_header_t) + hoffset + pconf->plen + op.bytes);

        ptr = (void *)pconf + sizeof(theora_packed_header_t);

        TRACE("Second header: len %d, err=%d\n", op.bytes, err);

        x_memcpy(ptr + hoffset + pconf->plen, op.packet, op.bytes);
        pconf->plen += op.bytes;
        ptr[2] = (op.bytes & 0xff);

        //      memset(&op, 0, sizeof(ogg_packet));
        err = th_encode_flushheader(ed, &cdc->tcmnt, &op);
        pconf = (theora_packed_header_t*) x_realloc(pconf,
                                                    sizeof(theora_packed_header_t) + hoffset + pconf->plen + op.bytes);

        ptr = (void *)pconf + sizeof(theora_packed_header_t);


        x_memcpy(ptr + hoffset + pconf->plen, op.packet, op.bytes);
        pconf->plen += op.bytes;

        TRACE("3D header: len %d, err=%d\n", op.bytes, err);

        cdc->header = pconf;
        cdc->header_len = sizeof(theora_packed_header_t) + hoffset + pconf->plen;

        pconf->plen = htons(pconf->plen);
        pconf->ident = htonl((1 << 4) | (cdc->ident << 8));

        edtmp = cdc->codec.ed;
        cdc->codec.ed = ed;
        if (edtmp)
        {
            th_encode_free(edtmp);
        }
        __x_theora_lastframe_update(cdc);
    }

    confbuf = x_malloc(cdc->header_len + 3);
    hdata = confbuf;

#ifdef DO_TYPE_CAST
    *((uint32_t *) hdata) = htonl(1); // numpkts
    hdata += 4;
#else
    write32(&hdata,htonl(1));
#endif

    x_memcpy(hdata, &cdc->header->ident, 3);
    hdata += 3;

    hdata[0] = 0xff & (cdc->header->plen);
    hdata[1] = 0xff & (cdc->header->plen >> 8);
    hdata += 2;

    ptr = (void *)pconf + sizeof(theora_packed_header_t);
    x_memcpy(hdata, ptr, cdc->header_len - 4);

    confstr = bin2hex(confbuf, cdc->header_len + 3);

#ifdef DEBUG
    fd = open ("theora_header_dmp.bin",O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
//    write(fd, '\n',1);
    write(fd, confbuf, cdc->header_len + 3);
    close(fd);

    fd = open ("theora_header_dmp.txt",O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
    write(fd, '\n',1);
    write(fd, confstr,2*(cdc->header_len + 3));
    close(fd);

    if (confbuf)
        x_free(confbuf);

    {
        int ll;
    confbuf = hex2bin(confstr,&ll);
    fd = open ("theora_header_dmp2.bin",O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
//    write(fd, '\n',1);
    write(fd, confbuf, ll);
    close(fd);
    }
#endif

    if (confstr)
    {
        _ASET(obj, _XS("configuration"), confstr);
        x_free(confstr);
    }

    if (confbuf)
        x_free(confbuf);

#endif
}

void
x_theora_context_init(x_object *obj, x_theora_context_t *cdc)
{
  memset(cdc, 0, sizeof(x_theora_context_t));
  th_info_init(&cdc->tinfo);
  th_comment_init(&cdc->tcmnt);
  th_setup_free(cdc->tsinfo);

  /** @todo Buggy jingle workaround: create fake encoder to generate 'confguration' attribute */
//  x_theora_encoder_init(obj,cdc);
}

void
x_theora_dispatch_header_packet(x_theora_context_t *cdc,
    const theora_packed_header_t *pconf)
{
  ogg_packet op;
  unsigned int hnum, plen;
  long l1;
  long l2;
  th_dec_ctx *ddtmp;
    int ident;
    int err;

  unsigned char *_hdata = (unsigned char *) pconf;

  th_info_init(&cdc->tinfo);
  th_comment_init(&cdc->tcmnt);

  ident = ntohl(read32(&_hdata)) >> 8;
  plen = ntohs(read16(&_hdata));

  hnum = (unsigned int)(char)_hdata[0] & 0xff;
  _hdata++;
  l1 = (long)(char)_hdata[0] & 0xff;
  _hdata++;
  l2 = (long)(char)_hdata[0] & 0xff;
  _hdata++;

  TRACE("ident 0x%x\n", ident);
  TRACE("plen %d\n", plen);

  /* start decoding */
  op.bytes = l1;
  op.packet =  _hdata;
  op.b_o_s = 1;
  op.packetno = 1;

  TRACE("op.bytes=%ld\n", op.bytes);

  err = th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);
  if (err<=0)
    return;

  op.bytes = l2;
  op.packet = _hdata + l1;
  op.b_o_s = 0;
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  err = th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);
  if (err<=0)
    return;

  op.bytes = plen - l2 - l1;
  op.packet = _hdata + l1 + l2;
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  err = th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);
  if (err<0)
    return;

  x_theora_decoder_init(cdc);

}

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
  int err;
  int fd;

  ENTER;

  th_info_init(&cdc->tinfo);
//  th_comment_init(&cdc->tcmnt);
  cdc->tsinfo = NULL;

  memset((void *) &op, 0, sizeof(ogg_packet));

    hdata = (unsigned char *) hex2bin(confstr, &hlen);
  /* try to decide base64 */
  if (!hdata)
    {
      hdata = (unsigned char *) base64_decode(confstr, strlen(confstr), &hlen);
      TRACE("Decoding base64: hdata=0x%p, hlen(%d)\n", hdata, hlen);
    }

//#ifdef DEBUG
#if 0
  ptr = hdata;
  fd = open("/sdcard/theora_header_dec.bin", O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
  write(fd, hdata, hlen);
  close(fd);

  fd = open ("/sdcard/theora_header_dec.txt",O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
//  write(fd, '\n',1);
  write(fd, confstr,x_strlen(confstr));
  close(fd);
#endif


#ifdef DO_TYPE_CAST
  numpkts = ntohl(*(uint32_t *) hdata);
  hdata += 4;

  cdc->ident = ntohl(*(uint32_t *) (hdata)) >> 8;
  hdata += 3; // For arm platform it should be aligned!!

  plen = (int) ntohs(*(uint16_t *) (hdata));
  hdata += 2;
#else
  numpkts = ntohl(read32(&hdata));
  cdc->ident = ntohl(read32(&hdata)) >> 8;
  --hdata; // get back by one symbol
  plen = (uint32_t) ntohs((uint16_t)read16(&hdata));
#endif

  TRACE("nupkts(%d),plen(%d),ident(0x%x),hdata(%p)\n",
        numpkts, plen, cdc->ident, hdata);

  hnum = (uint32_t) (char)hdata[0];
  hdata++;
  l1 = (uint32_t) (char)hdata[0];
  hdata++;
  l2 = (uint32_t) (char)hdata[0];
  hdata++;

  TRACE("hnum(%d),l1(%ld),l2(%ld)\n", hnum, l1, l2);

  /* start decoding */
  op.bytes = l1;
  op.packet = hdata;
  op.b_o_s = 1;
  op.packetno = 1;
  op.granulepos = -1;

  TRACE("op.bytes=%ld\n", op.bytes);

  err = th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);
  TRACE("err=%d\n", err);

  op.bytes = l2;
  op.packet = hdata + l1;
  op.b_o_s = 0;
  op.packetno++;

  TRACE("op.bytes=%ld\n", op.bytes);

  err = th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);
  TRACE("err=%d\n", err);

  op.bytes = plen - l2 - l1;
  op.packet = hdata + l1 + l2;
  op.packetno++;

  err = th_decode_headerin(&cdc->tinfo, &cdc->tcmnt, &cdc->tsinfo, &op);
  TRACE("err=%d\n", err);

  if (ptr)
    free(ptr);

  EXIT;
  return;

}

static void
x_theora_dispatch_frame_packet(x_theora_context_t *cdc,
    const theora_packed_header_t *pconf, int totlen, int mark)
{
  static int curpos = 0;
  static ogg_int64_t packetno = 3; /* skip header packets */
  int plen;
  int plen2;
  int err = -1;
  ogg_packet op;
  unsigned char *ptr = &((char *) pconf)[4];

  int numpacks = ((char *) (void *) pconf)[3] & 0xf;

//  plen = ntohs(read16(&ptr));
//  ptr-=2;
  plen = plen2 = read16(&ptr);

//  TRACE("Packet received: mark(%d), payloadlen(%d), plen2(%d) "
//  "packet=%p, data=%p, packetnum=%d, dataoffst=%d, totlen=%d\n",
//      mark, plen, plen2, pconf, &((char *) pconf)[0], numpacks, (int)(&((char *) pconf)[6] - (char *)(void *)pconf), totlen);

  switch (mark)
    {

  case 1:
    curpos = 0;
    memset(cdc->buffer, 0, cdc->bufsiz);

  case 2:
    if ((curpos + plen) > cdc->bufsiz)
      {
        cdc->bufsiz = curpos + plen;
        cdc->buffer = x_realloc(cdc->buffer, cdc->bufsiz);
      }
    memcpy(&cdc->buffer[curpos], &((char *) pconf)[6], plen);
    curpos += plen;
    return;

  case 0:
    curpos = 0;
  case 3:
    if ((curpos + plen) > cdc->bufsiz)
      {
        cdc->bufsiz = curpos + plen;
        cdc->buffer = x_realloc(cdc->buffer, cdc->bufsiz);
      }
    memcpy(&cdc->buffer[curpos], &((char *) pconf)[6], plen);
    curpos += plen;
    break;
    };

  op.bytes = (long) curpos;
  op.packet = (unsigned char *)(void *)cdc->buffer;
//  op.packetno = packetno++;
  op.packetno = 0;
  op.b_o_s = 0;
  op.e_o_s = 0;
  op.granulepos = -1;

//  TRACE("packet in Decoder(%p),Encoder(%d),state(%d)\n",
//        cdc->codec.dd,cdc->codec.ed, cdc->type);
  th_decode_packetin(cdc->codec.dd, &op, &op.granulepos);
//  TRACE("packet out\n");
  err = th_decode_ycbcr_out(cdc->codec.dd, cdc->lastframe);
//  TRACE("ok\n");

  if (err)
  {
      TRACE("ERROR decoding packet data\n");
      return;
  }
//  else
//      TRACE("Decode Ok, %p, bytes=%d\n", cdc->lastframe, curpos);

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

//  TRACE("OK! Theora payload: len=%d\n", len);

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
