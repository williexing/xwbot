/*
 * xhid_codec.c
 *
 *  Copyright (C) 2012, CrossLine Media, Ltd.
 *  http://www.clmedia.ru
 *
 *  Created on: Jul 28, 2011
 *      Author: CrossLine Media
 */
#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
#define DEBUG_PRFX "[xHID] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <rtp.h>

enum
{
    TYPE_UNSPECIFIED = -1,
    TYPE_ENCODER = 1,
    TYPE_DECODER = 2,
};

typedef struct xHID
{
    x_object obj;
    int mode;
    unsigned int seqno;
    time_t stamp;
} xHID_t;

#define PATH_MTU 1400
#define PATH_MTU_RTP (1400 - sizeof(rtp_hdr_t))

static int
x_hid_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int _dat2[4];
    x_object *tmp;
    xHID_t *thiz = (xHID_t *) (void *) o;
    int *_dat = (int *)buf;

    x_obj_attr_t hints = { 0, 0, 0 };

    ENTER;

    TRACE("Hid event len = %d\n", len >> 2);
    printf("Hid event len = %d\n", len >> 2);

    switch (thiz->mode)
    {
    case TYPE_DECODER:
        if (!o->write_handler)
        {
            TRACE("Hid eater not found\n");
            tmp = _PARNT(thiz);
            if (tmp)
                tmp = _CHLD(_PARNT(tmp),"hiddemux");
            if (tmp)
            {
                TRACE("No Hid eater... FATAL!!!\n");
                x_object_set_write_handler(X_OBJECT(o),X_OBJECT(tmp));
            }
        }
        if (o->write_handler)
        {
            TRACE("Push packet to decode len=%d\n", len);
            _dat2[0] = ntohl(_dat[0]);
            _dat2[1] = ntohl(_dat[1]);
            _dat2[2] = ntohl(_dat[2]);
            _dat2[3] = ntohl(_dat[3]);
            _WRITE(thiz->obj.write_handler, _dat2, sizeof(_dat2), NULL);
        }
        break;

    case TYPE_ENCODER:
        TRACE("Push packet to encode len=%d\n", len);
        _dat2[0] = htonl(_dat[0]);
        _dat2[1] = htonl(_dat[1]);
        _dat2[2] = htonl(_dat[2]);
        _dat2[3] = htonl(_dat[3]);
        setattr("pt", _GETNM(o), &hints);
        sprintf(buf, "%d", ++thiz->stamp);
        setattr("timestamp", buf, &hints);
        sprintf(buf, "%u", ++thiz->seqno);
        setattr("seqno", buf, &hints);
        _WRITE(thiz->obj.write_handler, _dat2, sizeof(_dat2), &hints);
        attr_list_clear(&hints);
        break;

    default:
        TRACE("Unknown codec mode\n");
        break;
    };
    EXIT;

    return len;
}

/* matching function */
static BOOL
x_hid_match(x_object *o, x_obj_attr_t *_o)
{
    /*const char *xmlns;*/
    ENTER;
    EXIT;
    return TRUE;
}

static int
x_hid_on_append(x_object *so, x_object *parent)
{
    xHID_t *pres = (xHID_t *) (void *) so;
    ENTER;
    so->write_handler = NULL;
    pres->seqno = ((int)so) % 0xffff;
    pres->stamp = ((int)so);
    EXIT;
    return 0;
}

static x_object *
x_hid_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
    xHID_t *thiz = (xHID_t *) this__;
    x_string_t md = getattr(X_PRIV_STR_MODE, attrs);

    if (!thiz->mode || TYPE_UNSPECIFIED == thiz->mode)
    {
        if (!md)
        {
            md = _AGET(this__,_XS(X_PRIV_STR_MODE));
            TRACE("Mode=%s\n",md);
        }

        if (!md)
        {
            TRACE("ERROR: Invalid codec state..\n");
        }
        else
        {
            if (md && EQ(md,"in"))
            {
                thiz->mode = TYPE_DECODER;
            }
            else if (md && EQ(md,"out"))
            {
                thiz->mode = TYPE_ENCODER;
            }
        }
    }

    TRACE("Codec mode %s\n", md);

    x_object_default_assign_cb(this__, attrs);

    return this__;
}

static void
x_hid_on_exit(x_object *this__)
{
    ENTER;
    printf("%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);
    EXIT;
}

static struct xobjectclass hid_codec_class;

static void
___presence_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *xhid;

    ENTER;

    xhid = x_object_add_path(obj, "__proc/hid/HID", NULL);
    _ASET(xhid, _XS("name"), _XS("HID"));
    _ASET(xhid, _XS("description"), _XS("Gobee HID device de-/payloader"));
    _ASET(xhid, _XS("classname"), _XS("HID"));
    _ASET(xhid, _XS("xmlns"), _XS("gobee:media"));

    EXIT;
}

static struct x_path_listener xhidc_bus_listener;

__x_plugin_visibility
__plugin_init void
x_hid_codec_init(void)
{
    ENTER;
    hid_codec_class.cname = "hid";
    hid_codec_class.psize = (sizeof(xHID_t) - sizeof(x_object));
    hid_codec_class.match = &x_hid_match;
    hid_codec_class.on_assign = &x_hid_on_assign;
    hid_codec_class.on_append = &x_hid_on_append;
    hid_codec_class.commit = &x_hid_on_exit;
    hid_codec_class.try_write = &x_hid_try_write;

    x_class_register_ns(hid_codec_class.cname, &hid_codec_class, _XS("gobee:media"));

    xhidc_bus_listener.on_x_path_event = &___presence_on_x_path_event;

    /**
       * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
       * namespace. So presence will be activated just after session xmpp is
       * activated.
       */
    x_path_listener_add("/", &xhidc_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_hid_codec_init)
;

