/*
 * x_vpx_plugin.c
 *
 *  Copyright (C) 2012, CrossLine Media, Ltd.
 *  http://www.clmedia.ru
 *
 *  Created on: Jul 28, 2011
 *      Author: CrossLine Media
 */

#undef DEBUG_PRFX
#include <x_config.h>
#define DEBUG_PRFX "[x_vpx] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include "x_vp8.h"
#include "mynet.h"


enum
{
    TYPE_UNSPECIFIED = -1,
    TYPE_ENCODER = 1,
    TYPE_DECODER = 2,
};

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
    xVPX_INITIALIZED = 0x10000
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
    union {
        vpx_codec_enc_cfg_t enccfg;
        vpx_codec_dec_cfg_t deccfg;
    };
    int type; // mode of codec (encoder/decoder)
    struct
    {
        int state;
        int id_counter;
        u_int32_t cr0;
    } regs;
    vpx_image_t lastframe;
    int seqno;
    long stamp;

    char *buffer;
    int bufsiz;
    int curpos;
} x_vpx_object_t;

static BOOL
x_vpx_equals(x_object *o, x_obj_attr_t *_o)
{
    ENTER;
    EXIT;
    return TRUE;
}

static int
x_vpx_on_append(x_object *thiz, x_object *parent)
{
    int fdin;
    int fdout;
    char _buf[64];
    ENTER;

    fdin = open(_ENV(thiz,"resource"), O_RDWR | O_CREAT);
    close(fdin);

    sprintf(_buf,"%s.vp8",_ENV(thiz,"resource"));
    fdin = open(_buf, O_RDWR | O_CREAT);
    close(fdout);

    EXIT;
    return 0;
}

static void
x_vpx_on_create(x_object *o)
{
    ENTER;
    EXIT;
}

static void
__vpx_yuv_set_data(vpx_image_t *frame, char *data)
{
    frame->planes[VPX_PLANE_Y] = data;
    frame->planes[VPX_PLANE_U] = data + frame->w * frame->h;
    frame->planes[VPX_PLANE_V] = frame->planes[VPX_PLANE_U] + frame->w * frame->h / 4;
}

static int
__x_vp8_rtp_send(x_object *thiz_, const void *buf, int count)
{
    char packbuf[MTU];
    //  unsigned char vp8hdr_buf[256];
    char _buf[48];
    int hdroff = 0;
    int len = 0;
    struct vp8_descriptor descriptor;
    int elapsed = count;
    int bufoff = 0;
    int capacity = 0;
    int _count = count;
    x_obj_attr_t hints =
    { 0, 0, 0 };
    x_vpx_object_t *thiz = (x_vpx_object_t *) thiz_;

    // pack first header
    descriptor.frame_beginning = 1;
    descriptor.non_reference_frame = 0;
    descriptor.picture_id = (thiz->seqno & ((1 << 7) - 1));
    descriptor.fragmentation_info = NOT_FRAGMENTED;
    hdroff = pack_vp8_descriptor(&descriptor, &packbuf[0], MTU);

    // update length
    len = hdroff;
    capacity = (MTU - len);
    elapsed = ((_count - capacity) > 0) ? (count - capacity) : 0;

    setattr("pt", "98", &hints);
    sprintf(_buf, "%d", thiz->stamp);
    setattr("timestamp", _buf, &hints);
    thiz->stamp += 3600;

    do
    {
        if (elapsed)
        {
            if (!bufoff)
            {
                // set fragmentation info of first packet
                packbuf[0] &= ~(0x3 << 1);
                packbuf[0] |= ((FIRST_FRAGMENT & 0x3) << 1);
                descriptor.fragmentation_info = FIRST_FRAGMENT;
            }
            else
            {
                // set fragmentation info of middle packet
                descriptor.frame_beginning = 0;
                descriptor.non_reference_frame = 0;
                descriptor.picture_id = __UINT32_MAX__;
                descriptor.fragmentation_info = MIDDLE_FRAGMENT;
                hdroff = pack_vp8_descriptor(&descriptor, &packbuf[0],
                        MTU);
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
            hdroff = pack_vp8_descriptor(&descriptor, &packbuf[0],
                    MTU);
        }
        else
        {
            //printf("0\n");
        }

        // update counters
        len = hdroff;
        capacity = (MTU - len);
        elapsed = ((_count - capacity) > 0) ? (_count - capacity) : 0;
        len += _count - elapsed;

        TRACE("Sending data! data=%x%x%x%x...\n",
              &((const char *) buf)[bufoff],
              &((const char *) buf)[bufoff+4],
              &((const char *) buf)[bufoff+8],
              &((const char *) buf)[bufoff+12]);

        memcpy(&packbuf[hdroff], &((const char *) buf)[bufoff], _count
               - elapsed);

        bufoff += _count - elapsed;
        _count = elapsed;
        elapsed = ((_count - capacity) > 0) ? (_count - capacity) : 0;

        sprintf(_buf, "%d", thiz->seqno++);
        setattr("seqno", _buf, &hints);

        TRACE("packet: elapsed(%d),len(%d), nexthop(%s)\n", elapsed,
              len,
              thiz_->write_handler ? _GETNM(thiz_->write_handler) : "(nil)");

        if (thiz_->write_handler)
            _WRITE(thiz_->write_handler, &packbuf[0], len, &hints);

    }
    while ((_count));

    attr_list_clear(&hints);

    return len;
}

static int
x_vpx_render_yuv_frame(x_vpx_object_t *thiz, vpx_image_t *img)
{
    int err = -1;
  x_iobuf_t planes[4];
  char buf[16];
  int stride;
  int len;
  x_obj_attr_t hints =
  { 0, 0, 0 };
  ENTER;
  
  /* pass to next hop (display) */
  X_IOV_DATA(&planes[0]) = img->planes[VPX_PLANE_Y];
  X_IOV_LEN(&planes[0]) = img->stride[VPX_PLANE_Y] * img->h;
  
  X_IOV_DATA(&planes[1]) = img->planes[VPX_PLANE_U];
  X_IOV_LEN(&planes[1]) = img->stride[VPX_PLANE_U] * img->h / 2;
  
  X_IOV_DATA(&planes[2]) = img->planes[VPX_PLANE_V];
  X_IOV_LEN(&planes[2]) = img->stride[VPX_PLANE_V] * img->h / 2;
  
  X_IOV_DATA(&planes[3]) = &img->stride[VPX_PLANE_Y];
  X_IOV_LEN(&planes[3]) = sizeof(img->stride[VPX_PLANE_Y]);
  
  if (thiz->lastframe.stride[0] != img->stride[0]
      || thiz->lastframe.w != img->w
      || thiz->lastframe.h != img->h)
  {
    x_sprintf(buf,"%d",img->w);
    setattr("width",buf,&hints);
    
    x_sprintf(buf,"%d",img->h);
    setattr("height",buf,&hints);
    
    x_sprintf(buf,"%d",img->stride);
    setattr("stride",buf,&hints);
    _ASGN(thiz->xobj.write_handler,&hints);
    
    // update local states
    thiz->lastframe.stride[0] = img->stride[0];
    thiz->lastframe.w = img->w;
    thiz->lastframe.h = img->h;
    
    // notify write handler
    _ASGN(thiz->xobj.write_handler,&hints);
    attr_list_clear(&hints);
  }
  
  if (_WRITEV(thiz->xobj.write_handler, planes, 4, NULL) < 0)
  {
      TRACE("Invalid nexthop(renderer)! Decreasing codec status...\n");
//    BUG();
//      len = img->h * img->stride * 3 / 2;
//      _WRITE(thiz->xobj.write_handler, planes[VPX_PLANE_Y], len, NULL);
  }
  else
  {
      err = 0; // ok
  }

  //    TRACE("Rendering...\n");
  EXIT;
  return err;
}

void
__on_vpx_packet_received(x_object *thiz_,
                         void *data, int len)
{
    x_vpx_object_t *thiz = (x_vpx_object_t *) thiz_;
    int off;
    vp8_descriptor_t vp8d;
    int ready = 0;
    int fdin;

    if (thiz->xobj.io_life_count < -DEFAULT_IOLIFECOUNT)
    {
        return;
    }

    if (!data)
        return;

    off = parse_vp8_packet(data, len, &vp8d);

    if (off <= 0)
        return;

    switch (vp8d.fragmentation_info)
    {
    case NOT_FRAGMENTED:
        TRACE("No fragment! data=%x%x%x%x...\n",
              ((char *)data)[off],
              ((char *)data)[off+4],
                ((char *)data)[off+8],
                ((char *)data)[off+12]);
        thiz->bufsiz =  len - off;
        thiz->buffer = x_realloc(thiz->buffer,thiz->bufsiz);
        memcpy(&thiz->buffer[0], &((char *)data)[off], (len - off));
        thiz->curpos = 0; // reset curpos
        ready = 1;
        break;

    case FIRST_FRAGMENT:
        TRACE("First frag! data=%x%x%x%x...\n",
              ((char *)data)[off],
              ((char *)data)[off+4],
                ((char *)data)[off+8],
                ((char *)data)[off+12]);
        thiz->bufsiz = (len - off);
        thiz->buffer = x_realloc(thiz->buffer,thiz->bufsiz);
        memcpy(&thiz->buffer[0], &((char *)data)[off], (len - off));
        thiz->curpos = thiz->bufsiz;
        break;

    case MIDDLE_FRAGMENT:
        TRACE("Middle frag! data=%x%x%x%x...\n",
              ((char *)data)[off],
              ((char *)data)[off+4],
                ((char *)data)[off+8],
                ((char *)data)[off+12]);
        thiz->bufsiz += (len - off);
        thiz->buffer = x_realloc(thiz->buffer,thiz->bufsiz);
        memcpy(&thiz->buffer[thiz->curpos], &((char *)data)[off], (len - off));
        thiz->curpos = thiz->bufsiz;
        break;

    case LAST_FRAGMENT:
        TRACE("Last frag! data=%x%x%x%x...\n",
              ((char *)data)[off],
              ((char *)data)[off+4],
                ((char *)data)[off+8],
                ((char *)data)[off+12]);
        thiz->bufsiz += (len - off);
        thiz->buffer = x_realloc(thiz->buffer,thiz->bufsiz);
        memcpy(&thiz->buffer[thiz->curpos], &((char *)data)[off], (len - off));
        thiz->curpos = 0;
        ready = 1;
        break;
    };

    TRACE(": %s %s, PIC-ID=%u\n", vp8d.frame_beginning ? "BEGINNING" : "",
          vp8d.non_reference_frame ? "REF" : "NO-REF",
          vp8d.frame_beginning ? vp8d.picture_id : 0);



    if (ready)
    {
        vpx_image_t *img;
        vpx_codec_err_t err = VPX_CODEC_OK;
        vpx_codec_iter_t iter = NULL;

        TRACE("Decoding frame: size(%d),from(%s)\n",thiz->bufsiz,_ENV(thiz,"resource"));
//        fdin = open(_ENV(thiz,"resource"), O_RDWR | O_APPEND);
//        write(fdin,thiz->buffer,thiz->bufsiz);
//        close(fdin);


        if (thiz->regs.cr0 & xVPX_INITIALIZED)
        {
            if (
                    (err=vpx_codec_decode(&thiz->vpx_codec, thiz->buffer, thiz->bufsiz, NULL, 0))
                    == VPX_CODEC_OK)
            {
                while ((img = vpx_codec_get_frame(&thiz->vpx_codec, &iter)))
                {
                    TRACE("Decoded frame(fmt(0x%x)) bps=%d,%dx%d\n", img->fmt, img->bps,
                          img->d_w, img->d_h);
                    if(x_vpx_render_yuv_frame(thiz, img) < 0)
                    {
                        thiz->xobj.io_life_count--;
                    }
                }
            }
            else
            {
                TRACE("DECODER ERROR: %d\n",err);
            }
        }
        else
        {
            TRACE("Decoder not ready!!!\n");
        }
    }
    return;
}



static int
x_vpx_writev(x_object *thiz_, x_iobuf_t *iov,
             u_int32_t iovlen, x_obj_attr_t *attr)
{
    x_string_t md;
    vpx_codec_iter_t iter;
    const vpx_codec_cx_pkt_t *pkt;
    int frame_cnt = 0;
    int flags = 0;
    x_vpx_object_t *thiz = (x_vpx_object_t *) thiz_;
    int fdout;
    char _buf[64];

    ENTER;

    switch (thiz->type)
    {
    case TYPE_DECODER:
        return -1; // NI
        break;

    case TYPE_ENCODER:
        if (iov && iovlen >= 3)
        {

          if (iovlen > 3)
          {

            vpx_img_alloc(&thiz->lastframe, VPX_IMG_FMT_I420,
                          thiz->enccfg.g_w, thiz->enccfg.g_h, 32);
            
            thiz->lastframe.stride[0] = *(int *)X_IOV_DATA(&iov[2]);
            thiz->lastframe.stride[1] = thiz->lastframe.stride[0] / 2;
            thiz->lastframe.stride[2] = thiz->lastframe.stride[0] / 2;
            
          }
 
          thiz->lastframe.planes[VPX_PLANE_Y] = X_IOV_DATA(&iov[0]);
          thiz->lastframe.planes[VPX_PLANE_U] = X_IOV_DATA(&iov[1]);
          thiz->lastframe.planes[VPX_PLANE_V] = X_IOV_DATA(&iov[2]);

//            thiz->lastframe.planes = X_IOV_DATA(&iov[0]);

            if (vpx_codec_encode(&thiz->vpx_codec,
                                 &thiz->lastframe, frame_cnt, 1, flags,
                                 VPX_DL_REALTIME ))
            {
                //                die_codec(&thiz->vpx_codec, "vpx_codec_enc_init()");
              TRACE("Encoder ERROR\n");
                BUG();
            }
            else
            {
              iter = NULL;
              while ((pkt = vpx_codec_get_cx_data(&thiz->vpx_codec, &iter)))
              {
                switch (pkt->kind)
                {
                  case VPX_CODEC_CX_FRAME_PKT:
                    __x_vp8_rtp_send((x_object *)thiz, (const void *) pkt->data.frame.buf,
                                     (int) pkt->data.frame.sz);
                    break;
                  default:
                    break;
                }
              }
            }
        }
        else
        {
            TRACE("ERROR: No data to encode!!!");
        }

        break;

    default:
        TRACE("Invalid CODEC State = %d\n", thiz->type);
        //        x_object_print_path(o,2);
        /* @todo FIXME This is a bug!!!! */
        md = _AGET(thiz_,_XS(X_PRIV_STR_MODE));
        if (md && EQ(md,"in"))
            thiz->type = TYPE_DECODER;
        else if (md && EQ(md,"out"))
            thiz->type = TYPE_ENCODER;
    };
    EXIT;
}

static int
x_vpx_write(x_object *thiz_, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    x_string_t md;
    vpx_codec_iter_t iter;
    const vpx_codec_cx_pkt_t *pkt;
    int frame_cnt = 0;
    int flags = 0;
    x_vpx_object_t *thiz = (x_vpx_object_t *) thiz_;
    ENTER;

    switch (thiz->type)
    {
    case TYPE_DECODER:
        __on_vpx_packet_received(thiz_, buf, len);
        break;
    case TYPE_ENCODER:
        if (buf && len > 0)
        {
            __vpx_yuv_set_data(&thiz->lastframe, buf);
            if (vpx_codec_encode(&thiz->vpx_codec,
                                 &thiz->lastframe, frame_cnt, 1, flags,
                                 VPX_DL_REALTIME ))
            {
                TRACE("Failed to encode frame: %s\n", vpx_codec_error(&thiz->vpx_codec));
                BUG();
            }

            iter = NULL;
            while ((pkt = vpx_codec_get_cx_data(&thiz->vpx_codec, &iter)))
            {
                switch (pkt->kind)
                {
                case VPX_CODEC_CX_FRAME_PKT:
                    __x_vp8_rtp_send((x_object *)thiz, (void *) pkt->data.frame.buf,
                                     (int) pkt->data.frame.sz);
                    break;
                default:
                    break;
                }
            }
        }
        else
        {
            TRACE("ERROR: No data to encode!!! buf(%p),len(%d)\n", buf, len);
        }

        break;
    default:
        TRACE("Invalid CODEC State = %d\n", thiz->type);
        //        x_object_print_path(o,2);
        /* @todo FIXME This is a bug!!!! */
        md = _AGET(thiz_,_XS(X_PRIV_STR_MODE));
        if (md && EQ(md,"in"))
            thiz->type = TYPE_DECODER;
        else if (md && EQ(md,"out"))
            thiz->type = TYPE_ENCODER;
    };
    EXIT;

    return len;
}

static x_object *
x_vpx_on_assign(x_object *thiz_, x_obj_attr_t *attrs)
{
    register x_obj_attr_t *attr;
    x_vpx_object_t *thiz = (x_vpx_object_t *) thiz_;
    x_string_t md = getattr(X_PRIV_STR_MODE, attrs);

    ENTER;

    if (thiz->type != TYPE_DECODER
            && thiz->type != TYPE_ENCODER)
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

  TRACE("Codec mode %s, state(%d)\n", md ? md : "(null)", thiz->type);

    x_object_default_assign_cb(thiz, attrs);

    if (md && EQ(md,"in"))
    {
        thiz->type = TYPE_DECODER;
        for (attr = attrs->next; attr; attr = attr->next)
        {
            if (EQ(attr->key,X_STRING("width")))
            {
                thiz->deccfg.w = atoi(attr->val);
              thiz->lastframe.w = thiz->deccfg.w;
                thiz->regs.cr0 |= xVPX_HAVE_W;
            }
            else if (EQ(attr->key,X_STRING("height")))
            {
                thiz->deccfg.h = atoi(attr->val);
              thiz->lastframe.h = thiz->deccfg.h;
              thiz->regs.cr0 |= xVPX_HAVE_H;
            }
        }

        //      if (!thiz->deccfg.threads) thiz->deccfg.threads = 4;

        if (!xVPX_IS_INITIALIZED(thiz->regs.cr0))
        {
            TRACE("!xVPX_IS_INITIALIZED(thiz->regs.cr0)\n");
            thiz->deccfg.threads = 4;
            if (/*xVPX_IS_CONFIGURED(thiz->regs.cr0)
                          &&*/ vpx_codec_dec_init(&thiz->vpx_codec,
                                                  (vpx_codec_vp8_dx()), NULL /*&thiz->deccfg*/,
                                                  0))
            {
                TRACE("Failed to init config: %s\n", vpx_codec_error(&thiz->vpx_codec));
                BUG();
            }
            else
            {
                TRACE("!xVPX_IS_CONFIGURED(thiz->regs.cr0)\n");
                thiz->regs.cr0 |= xVPX_INITIALIZED;
            }
        }
        else
            TRACE("xVPX_IS_INITIALIZED(thiz->regs.cr0)\n");

    }
    else if (md && EQ(md,"out"))
    {
        thiz->type = TYPE_ENCODER;
        for (attr = attrs->next; attr; attr = attr->next)
        {
            x_object_setattr(thiz_, attr->key, attr->val);
            if (EQ(attr->key,X_STRING("bitrate")))
            {
                thiz->enccfg.rc_target_bitrate = atoi(attr->val);
            }
            else if (EQ(attr->key,X_STRING("width")))
            {
                thiz->enccfg.g_w = atoi(attr->val);
              thiz->regs.cr0 |= xVPX_HAVE_W;
            }
            else if (EQ(attr->key,X_STRING("height")))
            {
                thiz->enccfg.g_h = atoi(attr->val);
                thiz->regs.cr0 |= xVPX_HAVE_H;
            }
            else if (EQ(attr->key,X_STRING("threadnum")))
            {
                thiz->enccfg.g_threads = atoi(attr->val);
            }
            else if (EQ(attr->key,X_STRING("kf_max")))
            {
                thiz->enccfg.kf_max_dist = atoi(attr->val);
            }
            else if (EQ(attr->key,X_STRING("kf_min")))
            {
                thiz->enccfg.kf_min_dist = atoi(attr->val);
            }
            else if (EQ(attr->key,X_STRING("kf_mode")))
            {
                thiz->enccfg.kf_mode = atoi(attr->val);
            }
        }

        if (!xVPX_IS_INITIALIZED(thiz->regs.cr0))
        {
            x_memset(&thiz->enccfg, 0, sizeof(thiz->enccfg));
            if(vpx_codec_enc_config_default(
                        vpx_codec_vp8_cx(), &thiz->enccfg, 0))
            {
                TRACE("Failed to get config: %s\n", vpx_codec_error(&thiz->vpx_codec));
                BUG();
            }

            thiz->enccfg.g_profile = 0;
            if (!thiz->enccfg.kf_max_dist) thiz->enccfg.kf_max_dist = 30;
            //x_codec->cfg.kf_min_dist = 0;
            if (!thiz->enccfg.g_threads) thiz->enccfg.g_threads = 4;
            if (!thiz->enccfg.g_pass) thiz->enccfg.g_pass = VPX_RC_ONE_PASS;
            if (!thiz->enccfg.rc_end_usage) thiz->enccfg.rc_end_usage = VPX_CBR;
            if (thiz->enccfg.rc_end_usage == VPX_CBR)
            {
                thiz->enccfg.rc_buf_initial_sz = 2000;
                thiz->enccfg.rc_buf_optimal_sz = 2000;
                thiz->enccfg.rc_buf_sz = 3000;
            }

            if (!thiz->enccfg.g_w)
            {
                thiz->enccfg.g_w = 320;
                thiz->regs.cr0 |= xVPX_HAVE_W;
            }

            if (!thiz->enccfg.g_h)
            {
                thiz->enccfg.g_h = 240;
                thiz->regs.cr0 |= xVPX_HAVE_H;
            }

            if (xVPX_IS_CONFIGURED(thiz->regs.cr0)
                    && vpx_codec_enc_init(&thiz->vpx_codec,
                                          (vpx_codec_vp8_cx()), &thiz->enccfg, 0))
            {
                TRACE("Failed to init config: %s\n", vpx_codec_error(&thiz->vpx_codec));
                BUG();
            }
            else
            {
                thiz->regs.cr0 |= xVPX_INITIALIZED;
                vpx_img_alloc(&thiz->lastframe, VPX_IMG_FMT_I420,
                              thiz->enccfg.g_w, thiz->enccfg.g_h, 0);
                TRACE("New image allocated: %d, plane=%p\n",
                      thiz->lastframe.fmt, thiz->lastframe.planes[0]);
            }
        }
        else
        {
            x_memset(&thiz->vpx_codec, 0, sizeof(thiz->vpx_codec));
            if (vpx_codec_enc_config_set(&thiz->vpx_codec, &thiz->enccfg))
            {
                TRACE("Failed to init config: %s\n", vpx_codec_error(&thiz->vpx_codec));
            }
        }
    }
    else
    {
        TRACE("What this? Codec mode='%s'\n",md ? md : "(null)");
    }

    EXIT;
    return thiz_;
}


static void
x_vpx_exit(x_object *o)
{
    x_vpx_object_t *x_codec = (x_vpx_object_t *) o;
    ENTER;
    printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
    if (x_codec->type == TYPE_ENCODER)
    {
        vpx_img_free(&x_codec->lastframe);
    }
    EXIT;
}

static struct xobjectclass x_vpx_class;

static void
__vpx_on_x_path_event(x_object *obj)
{
    x_object *tmp;
    ENTER;

    tmp = x_object_add_path(obj, "__proc/video/VP8", NULL);
    _ASET(tmp,"description", "VP8 Video codec");
    _ASET(tmp,"name", "vp8");
    _ASET(tmp, _XS("classname"), _XS("vp8"));
    _ASET(tmp, _XS("xmlns"), _XS("gobee:media"));

    EXIT;
}

static x_path_listener_t vpx_bus_listener;

__x_plugin_visibility
__plugin_init void
x_vpx_init(void)
{
    ENTER;

    printf("VP8 ABI Version = %d\n", VPX_ENCODER_ABI_VERSION);

    x_vpx_class.cname = "vp8";
    x_vpx_class.psize = sizeof(x_vpx_object_t) - sizeof(x_object);
    x_vpx_class.on_create = &x_vpx_on_create;
    x_vpx_class.match = &x_vpx_equals;
    x_vpx_class.on_assign = &x_vpx_on_assign;
    x_vpx_class.on_append = &x_vpx_on_append;
    x_vpx_class.commit = &x_vpx_exit;
    x_vpx_class.try_writev = &x_vpx_writev;
    x_vpx_class.try_write = &x_vpx_write;


    x_class_register_ns(x_vpx_class.cname, &x_vpx_class, _XS("gobee:media"));

    vpx_bus_listener.on_x_path_event = (void
                                        (*)(void*)) &__vpx_on_x_path_event;

    x_path_listener_add("/",&vpx_bus_listener);

    EXIT;
}

PLUGIN_INIT(x_vpx_init);

