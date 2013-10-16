/*
 * xtheora_plugin.c
 *
 *  Copyright (C) 2012, CrossLine Media, Ltd.
 *  http://www.clmedia.ru
 *
 *  Created on: Jul 28, 2011
 *      Author: CrossLine Media
 */
#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_XVIDEO_THEORA_ON
#define DEBUG_PRFX "[xTHEORA] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <rtp.h>

#include "x_theora_api.h"

typedef struct xTHEORA
{
    x_object obj;
    unsigned int ywidth;
    unsigned int yheight;
    unsigned int stride;
    struct x_theora_context cdc_ctx;
} xTHEORA_t;

#define PATH_MTU 1300
#define PATH_MTU_RTP (1300 - sizeof(rtp_hdr_t))
#define PATH_MTU_THEORA (PATH_MTU_RTP - THEORA_HDR_SIZE)

static unsigned int seqno = 0;
static time_t stamp = 123123;
static int header_cnt = 0;
static int tot_header_cnt = 1000;

static unsigned char *
write32(unsigned char **data_, uint32_t val)
{
    unsigned char *data = *data_;
    data[3] = (val >> 24)& 0xff;
    data[2] = (val >> 16)& 0xff;
    data[1] = (val >> 8)& 0xff;
    data[0] = val& 0xff;
    *data_ += 4;
    return *data_;
}

static unsigned char *
write16(unsigned char **data_, uint32_t val)
{
    unsigned char *data = *data_;
    data[1] = (val >> 8)& 0xff;
    data[0] = val& 0xff;
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

static void
__theora_send_fragmented_rtp(x_object *o, const char *packet, int len,
                             long stamp)
{
    char packbuf[PATH_MTU];
    char buf[48];
    int elapsed = 0;
    int _len;
    int sent = 0;
    theora_packed_header_t *dataout = (theora_packed_header_t *) packbuf;
    int mark = 0;
    int tdt = 0;
    int numpackets = 1;
    unsigned char *ptr;
    x_obj_attr_t hints =
    { 0, 0, 0 };
    xTHEORA_t *pres = (xTHEORA_t *) (void *) o;

    /*
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                     Ident                     | F |TDT|# pkts.|
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   *
   * F: Fragment type (0=none, 1=start, 2=cont, 3=end)
   * TDT: Theora data type (0=theora, 1=config, 2=comment, 3=reserved)
   * pkts: number of packets.
   */

    TRACE("Sending... len(%d), MTU(%d) bytes\n", len, PATH_MTU_THEORA);

    memset(dataout, 0, sizeof(theora_packed_header_t));

    setattr("pt", "96", &hints);
    sprintf(buf, "%d", stamp);
    setattr("timestamp", buf, &hints);

    if (len > PATH_MTU_THEORA)
        numpackets = 0;

    for (sent = 0, elapsed = len; elapsed > 0; sent += _len, elapsed -= _len)
    {
        _len = PATH_MTU_THEORA < elapsed ? PATH_MTU_THEORA : elapsed;

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

        /* payload now points to a length with that many theora data bytes.
       * Iterate over the packets and send them out.
       *
       *  0                   1                   2                   3
       *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |             length            |          theora data         ..
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * ..                        theora data                           |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * |            length             |   next theora packet data    ..
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       * ..                        theora data                           |
       * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+*
       */
#ifdef DO_TYPE_CAST
        *(uint32_t *) &packbuf[0] = htonl((pres->cdc_ctx.ident << 8));
        *(uint16_t *) &packbuf[4] = htons((short) (_len & 0xffff));
#else
        ptr = (unsigned char *)packbuf;
        write32(&ptr, htonl((pres->cdc_ctx.ident << 8)));
        write16(&ptr, htons((short) (_len & 0xffff)));
#endif

        packbuf[3] |= (char) (((mark & 0x3) << 6) | ((tdt & 0x3) << 4)
                              | (numpackets & 0xf));

        memcpy(&packbuf[6], &packet[sent], _len);

        sprintf(buf, "%u", seqno++);
        setattr("seqno", buf, &hints);

        TRACE("packet: elapsed(%d), sent(%d),"
              " _len(%d), netxthop(%p) = '%s'\n", elapsed,
              sent, _len, o->write_handler,
              o->write_handler ? _GETNM(o->write_handler) : "(nil)");

        // FIXME we allocate here space for rtp header t obe used later in stack downstream
        //      _WRITE(o->write_handler, &packbuf[0], THEORA_HDR_SIZE + _len, &hints);
        if (o->write_handler)
            _WRITE(o->write_handler, &packbuf[0], 6 + _len, &hints);

    }

    attr_list_clear(&hints);

}

static void
__gobee_yuv_set_data(gobee_img_frame_t yuv, void *data)
{
    yuv[0].data = data;
    yuv[1].data = &yuv[0].data[0]
            + yuv[0].stride * yuv[0].height;
    yuv[2].data = &yuv[1].data[0]
            + yuv[1].stride * yuv[1].height;
}

static void
__theora_encode_and_send_yuv(x_object *o, th_ycbcr_buffer *yuv)
{
    int err;
    ogg_packet op;
    struct th_enc_ctx *ed;
    xTHEORA_t *pres = (xTHEORA_t *) (void *) o;

    ed = (struct th_enc_ctx *) pres->cdc_ctx.codec.ed;

    err = th_encode_ycbcr_in(pres->cdc_ctx.codec.ed, yuv);
    TRACE("---> Encode packet err=%d, encoder(%p) [%dx%d][%dx%d][%dx%d]\n",
          err,
          pres->cdc_ctx.codec.ed,
          (*yuv)[0].width,
          (*yuv)[0].height,
          (*yuv)[1].width,
          (*yuv)[1].height,
          (*yuv)[2].width,
          (*yuv)[2].height);

    if ((err = th_encode_packetout(pres->cdc_ctx.codec.ed, 0, &op)) > 0)
    {
        TRACE("Sending packet...\n");
        //      if (op.bytes > 0)
        __theora_send_fragmented_rtp(o, op.packet, op.bytes, stamp);
        //      else
        //        TRACE("No data to send\n");
        stamp += 6670;
    }
    else
    {
        TRACE("Error encoding packet, %d\n", err);
    }
}

static void
__theora_encode_and_send(x_object *o, void *rawframe, u_int32_t framelen)
{
    int err;
    ogg_packet op;
    struct th_enc_ctx *ed;
    xTHEORA_t *pres = (xTHEORA_t *) (void *) o;

    ed = (struct th_enc_ctx *) pres->cdc_ctx.codec.ed;

    __gobee_yuv_set_data(pres->cdc_ctx.lastframe, rawframe);

    return __theora_encode_and_send_yuv(o,pres->cdc_ctx.lastframe);
}

static void
__x_theora_send_headers(x_object *o)
{
    char buf[64];
    ogg_packet op;
//    theora_packed_header_t *pconf = 0;
    xTHEORA_t *pres = (xTHEORA_t *) (void *) o;
    x_obj_attr_t hints =
    { 0, 0, 0 };
    int hoffset = 3;
//    unsigned char _hdata[4096];
    unsigned char *_hdata;
    int plen;
    int err;

    struct x_theora_context *cdc = &pres->cdc_ctx;

#if 1
    TRACE("%s:%p\n",
          cdc->type == TYPE_ENCODER?
              "ENCODER" : cdc->type == TYPE_DECODER?
                  "DECODER" : "UNKNOWN TYPE", pres->cdc_ctx.codec.ed);
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
    TRACE("quality(%d)\n", cdc->tinfo.quality);
#endif

    /*  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                   Ident                       | length       ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              | n. of headers |    length1    |    length2   ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              |             Identification Header            ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..              |         Comment Header                       ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                        Comment Header                        |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                          Setup Header                        ..
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * .................................................................
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * ..                         Setup Header                         |
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */

//    if (!pres->cdc_ctx.header)
//    {

        memset(&op, 0, sizeof(ogg_packet));
        err = th_encode_flushheader(pres->cdc_ctx.codec.ed, &pres->cdc_ctx.tcmnt, &op);

        if(err>0)
        {

            TRACE("First header: len %d\n", op.bytes);

            if(!pres->cdc_ctx.header)
            {
                pres->cdc_ctx.header = x_malloc(4096);
            }

            _hdata = pres->cdc_ctx.header;

            write32(&_hdata,htonl(((0 & 0x3) << 6) /*fragmentation*/
                                  | ((1 & 0x7) << 4) /*TDT*/
                                  | (1 & 0x7)/*numpackets*/
                                  | (pres->cdc_ctx.ident << 8) /*ident*/));

            // skip plen
            _hdata+=2;

            _hdata[0] = (char)(2);
            _hdata++;
            // l1
            _hdata[0] = (char)op.bytes;
            _hdata++;
            memcpy(_hdata + 1, op.packet, op.bytes);

            plen = op.bytes;

            th_encode_flushheader(pres->cdc_ctx.codec.ed, &pres->cdc_ctx.tcmnt, &op);

            TRACE("Second header: len %d\n", op.bytes);
            _hdata[0] = (char)op.bytes;
            _hdata++;
            memcpy(_hdata + plen, op.packet, op.bytes);

            plen += op.bytes;

            th_encode_flushheader(pres->cdc_ctx.codec.ed, &pres->cdc_ctx.tcmnt, &op);
            memcpy(_hdata + plen, op.packet, op.bytes);

            plen += op.bytes;

            TRACE("Third header: len %d\n", op.bytes);

            //        pres->cdc_ctx.header = _hdata;
            pres->cdc_ctx.header_len = THEORA_HDR_SIZE /* ident+plen */
                    + hoffset + plen;

            //        pconf->plen = htons(pconf->plen);
            _hdata = pres->cdc_ctx.header;
            _hdata+=4;
            write16(&_hdata,htons(plen));
        }
        //    }
//    else
//    {
//        TRACE("Header already generated\n");
//    }

    if (o->write_handler && pres->cdc_ctx.header)
    {
        TRACE("Sending HEADERS: overall size: %d\n", pres->cdc_ctx.header_len);

        setattr("pt", "96", &hints);

        sprintf(buf, "%d", stamp);
        setattr("timestamp", buf, &hints);

        sprintf(buf, "%u", seqno++);
        setattr("seqno", buf, &hints);

        _WRITE(o->write_handler, (void *) pres->cdc_ctx.header,
               pres->cdc_ctx.header_len, &hints);
    }
}

void
x_theora_render_yuv_frame(x_theora_context_t *ctx, th_ycbcr_buffer ycbcr)
{
    int fd;
    int len;
    char buf[16];
    x_iobuf_t planes[3];
    xTHEORA_t *thiz = container_of(ctx,((xTHEORA_t *)0),cdc_ctx);
    x_obj_attr_t hints =
    { 0, 0, 0 };

//    TRACE("New frame ready (%p),0{%d,%d,%d,%p},"
//          "1{%d,%d,%d,%p},"
//          "2{%d,%d,%d,%p}... playing\n",
//          ycbcr, ycbcr[0].width, ycbcr[0].height, ycbcr[0].stride, ycbcr[0].data, ycbcr[1].width, ycbcr[1].height, ycbcr[1].stride, ycbcr[1].data, ycbcr[2].width, ycbcr[2].height, ycbcr[2].stride, ycbcr[2].data);

    if (thiz->ywidth != ycbcr[0].width
            || thiz->yheight != ycbcr[0].height
            || thiz->stride != ycbcr[0].stride)
    {
        x_sprintf(buf,"%d",ycbcr[0].width);
        setattr("width",buf,&hints);

        x_sprintf(buf,"%d",ycbcr[0].height);
        setattr("height",buf,&hints);

        x_sprintf(buf,"%d",ycbcr[0].stride);
        setattr("stride",buf,&hints);
        _ASGN(thiz->obj.write_handler,&hints);

        // update local states
        thiz->ywidth = ycbcr[0].width;
        thiz->yheight = ycbcr[0].height;
        thiz->stride = ycbcr[0].stride;

        // notify write handler
        _ASGN(thiz->obj.write_handler,&hints);
        attr_list_clear(&hints);
    }

    /* pass to next hop (display) */

    X_IOV_DATA(&planes[0]) = ycbcr[0].data;
    X_IOV_LEN(&planes[0]) = ycbcr[0].height * ycbcr[0].stride;

    X_IOV_DATA(&planes[1]) = ycbcr[1].data;
    X_IOV_LEN(&planes[1]) = ycbcr[1].height * ycbcr[1].stride;

    X_IOV_DATA(&planes[2]) = ycbcr[2].data;
    X_IOV_LEN(&planes[2]) = ycbcr[2].height * ycbcr[2].stride;

    TRACE("Rendering...\n");

    if (_WRITEV(thiz->obj.write_handler, planes, 3, NULL) < 0)
    {
        len = ycbcr[0].height * ycbcr[0].stride * 3 / 2;
        _WRITE(thiz->obj.write_handler, ycbcr[0].data, len, NULL);
    }

//    TRACE("Rendering...\n");

    /*
   fd = open("yuv.in.dump", O_APPEND | O_WRONLY);
   write(fd, ycbcr[0].data, len);
   close(fd);
   */
    EXIT;
}


static int
x_theora_try_writev(x_object *o, x_iobuf_t *iov,
                    u_int32_t iovlen, x_obj_attr_t *attr)
{
    x_string_t md;
    xTHEORA_t *pres = (xTHEORA_t *) (void *) o;

    ENTER;

    switch (pres->cdc_ctx.type)
    {
    case TYPE_DECODER:
        return -1; // NI
        break;

    case TYPE_ENCODER:
        if (--header_cnt < 0 && --tot_header_cnt > 0)
        {
            __x_theora_send_headers(o);
            header_cnt = 100;
        }

        if (iov && iovlen == 3)
        {
            pres->cdc_ctx.lastframe[0].data = X_IOV_DATA(&iov[0]);
            pres->cdc_ctx.lastframe[1].data = X_IOV_DATA(&iov[1]);
            pres->cdc_ctx.lastframe[2].data = X_IOV_DATA(&iov[2]);
            __theora_encode_and_send_yuv(o,pres->cdc_ctx.lastframe);
        }
        else
        {
            TRACE("ERROR: No data to encode!!!\n");
        }
        break;
    default:
        TRACE("Invalid CODEC State = %d\n", pres->cdc_ctx.type);
        //        x_object_print_path(o,2);
        /* @todo FIXME This is a bug!!!! */
        md = _AGET(o,_XS(X_PRIV_STR_MODE));
        if (md && EQ(md,"in"))
            pres->cdc_ctx.type = TYPE_DECODER;
        else if (md && EQ(md,"out"))
            pres->cdc_ctx.type = TYPE_ENCODER;
    };
    EXIT;
}


static int
x_theora_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    x_string_t md;
    xTHEORA_t *pres = (xTHEORA_t *) (void *) o;
    ENTER;

    switch (pres->cdc_ctx.type)
    {
    case TYPE_DECODER:
//        TRACE("Push packet to decode len=%d\n", len);
        __on_theora_packet_received(&pres->cdc_ctx, buf, len);
        break;
    case TYPE_ENCODER:
        if (--header_cnt < 0 && --tot_header_cnt > 0)
        {
            __x_theora_send_headers(o);
            header_cnt = 100;
        }

        if (buf && len > 0)
        {
            __gobee_yuv_set_data(pres->cdc_ctx.lastframe, buf);
            __theora_encode_and_send_yuv(o,pres->cdc_ctx.lastframe);
        }
        else
        {
            TRACE("ERROR: No data to encode!!! buf(%p),len(%d)\n", buf, len);
        }
        break;
    default:
        TRACE("Invalid CODEC State = %d\n", pres->cdc_ctx.type);
        //        x_object_print_path(o,2);
        /* @todo FIXME This is a bug!!!! */
        md = _AGET(o,_XS(X_PRIV_STR_MODE));
        if (md && EQ(md,"in"))
            pres->cdc_ctx.type = TYPE_DECODER;
        else if (md && EQ(md,"out"))
            pres->cdc_ctx.type = TYPE_ENCODER;
    };
    EXIT;

    return len;
}

/* matching function */
static BOOL
theora_match(x_object *o, x_obj_attr_t *_o)
{
    /*const char *xmlns;*/
    ENTER;

    EXIT;
    return TRUE;
}

static int
theora_on_append(x_object *_thiz, x_object *parent)
{
    xTHEORA_t *thiz = (xTHEORA_t *) (void *) _thiz;
    ENTER;

    _thiz->write_handler = NULL;

    thiz->cdc_ctx.type = TYPE_UNSPECIFIED;

    // set default values
    _ASET(_thiz, "width", "320");
    _ASET(_thiz, "height", "240");
    _ASET(_thiz, "stride", "320");
    _ASET(_thiz, "type", "yuv420");

    EXIT;
    return 0;
}

static void
theora_on_remove(x_object *so, x_object *parent)
{
    xTHEORA_t *pres = (xTHEORA_t *) (void *) so;
    ENTER;

    _ASET(so,_XS(X_PRIV_STR_MODE),NULL);
    pres->cdc_ctx.type = TYPE_UNSPECIFIED;

    _REFPUT(so->write_handler, NULL);
    so->write_handler = NULL;

    EXIT;
}

static x_object *
theora_on_assign(x_object *thiz, x_obj_attr_t *attrs)
{
    xTHEORA_t *pres = (xTHEORA_t *) thiz;
    const char *conf = NULL;
    x_string_t md = getattr(X_PRIV_STR_MODE, attrs);

    if (pres->cdc_ctx.type != TYPE_DECODER
            && pres->cdc_ctx.type != TYPE_ENCODER)
    {
        if (!md)
        {
            md = _AGET(thiz,_XS(X_PRIV_STR_MODE));
            TRACE("Mode=%s\n",md);
        }

        if (!md)
        {
            TRACE("ERROR: Invalid codec state..\n");
        }
    }

    TRACE("Codec mode %s, state(%d)\n", md, pres->cdc_ctx.type);

    x_object_default_assign_cb(thiz, attrs);

    if (md && EQ(md,"in"))
    {
        pres->cdc_ctx.type = TYPE_DECODER;
//        if (getattr(X_PRIV_STR_WIDTH, attrs))
//        {
//        }
    }
    else if (md && EQ(md,"out"))
    {
        pres->cdc_ctx.type = TYPE_ENCODER;
        if (!pres->cdc_ctx.codec.ed)
        {
            x_theora_context_init(thiz, &pres->cdc_ctx);
            x_theora_encoder_init(thiz, &pres->cdc_ctx);
        }
        else if (getattr(X_PRIV_STR_WIDTH, attrs))
        {
            x_theora_context_init(thiz, &pres->cdc_ctx);
            x_theora_encoder_init(thiz, &pres->cdc_ctx);
            __x_theora_send_headers(thiz);
        }
    }
    else
    {
        TRACE("What this? Codec mode='%s'\n",md);
    }

    if (pres->cdc_ctx.type == TYPE_DECODER
            && !pres->cdc_ctx.codec.dd)
    {
        conf = getattr("configuration", attrs);
        if (!conf)
            conf = _AGET(thiz,"configuration");

        if (conf)
        {
            TRACE("Reading theora decoder configuration '%s'\n",conf);
            theora_read_str_config(&pres->cdc_ctx, conf);
            //      if(!pres->cdc_ctx.codec.dd)
            //      {
            TRACE("Initializing theora decoder with '%s'\n",conf);
            x_theora_decoder_init(&pres->cdc_ctx);

//            __gobee_yuv_set_data(&pres->cdc_ctx.lastframe,
//                                 x_malloc(pres->cdc_ctx.lastframe[0].width * pres->cdc_ctx.lastframe[0].height * 3));
            //      }

        }
    }

    //  x_theora_chkstate(pres);

    return thiz;
}

static void
theora_exit(x_object *this__)
{
    ENTER;
    printf("%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
    EXIT;
}

static struct xobjectclass theora_classs;

static void
___presence_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *theora;

    ENTER;

    theora = x_object_add_path(obj, "__proc/video/THEORA", NULL);
    _ASET(theora, _XS("name"), _XS("THEORA"));
    _ASET(theora, _XS("description"), _XS("Theora video codec"));
    _ASET(theora, _XS("classname"), _XS("theora"));
    _ASET(theora, _XS("xmlns"), _XS("gobee:media"));

    EXIT;
}

static struct x_path_listener theora_bus_listener;

__x_plugin_visibility
__plugin_init void
x_theora_plugin_init(void)
{
    ENTER;
    theora_classs.cname = "theora";
    theora_classs.psize = (sizeof(xTHEORA_t) - sizeof(x_object));
    theora_classs.match = &theora_match;
    theora_classs.on_assign = &theora_on_assign;
    theora_classs.on_append = &theora_on_append;
    theora_classs.on_remove = &theora_on_remove;
    theora_classs.commit = &theora_exit;
    theora_classs.try_write = &x_theora_try_write;
    theora_classs.try_writev = &x_theora_try_writev;

    x_class_register_ns(theora_classs.cname, &theora_classs, _XS("gobee:media"));

    theora_bus_listener.on_x_path_event = &___presence_on_x_path_event;

    /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
    x_path_listener_add("/", &theora_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_theora_plugin_init)
;

