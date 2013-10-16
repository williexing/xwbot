/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-QVHID] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#define X_IN_DEVDIR "__proc/in"
#define HID_VENDOR_ID "qvfb_HID_Device"

#include "xqvhid.h"

typedef struct x_qvhid
{
    x_object xobj;
    int keypipe;
    int mousepipe;
    //  int dId;
} x_qvhid_t;

extern int qvfb_get_key_pipe(KEY sid);
extern int qvfb_get_mouse_pipe(KEY sid);

#if 0
void QVFbKeyPipeProtocol::sendKeyboardData(QString unicode, int keycode,
                                           int modifiers, bool press, bool repeat)
{
    QVFbKeyData kd;
    kd.unicode = unicode[0].unicode();
    kd.keycode = keycode;
    kd.modifiers = static_cast<Qt::KeyboardModifier>(modifiers);
    kd.press = press;
    kd.repeat = repeat;
    write(fd, &kd, sizeof(QVFbKeyData));
}
#endif

static int
x_qvhid_on_append(x_object *so, x_object *parent)
{
    x_qvhid_t *hidctl = (x_qvhid_t *) so;
    ENTER;

    TRACE("\n");

    //  hidctl->dId = qvfb_gen_id(_ENV(so,"sid"));
    hidctl->keypipe = qvfb_get_key_pipe(_ENV(so,"sid"));
    hidctl->mousepipe = qvfb_get_mouse_pipe(_ENV(so,"sid"));

    _ASET(so,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
    _ASET(so,_XS("Descr"),_XS("Virtual HID Device (Qvfb)"));

    EXIT;
    return 0;
}

static x_object *
x_qvhid_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
    x_qvhid_t *mctl = (x_qvhid_t *) this__;
    ENTER;

    x_object_default_assign_cb(this__, attrs);

    EXIT;
    return this__;
}

static void
x_qvhid_exit(x_object *this__)
{
    x_qvhid_t *mctl = (x_qvhid_t *) this__;
    ENTER;
    printf("%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);

    if (mctl->xobj.write_handler)
    {
        _REFPUT(mctl->xobj.write_handler, NULL);
        mctl->xobj.write_handler = NULL;
    }

    EXIT;
}


static int
x_qvhid_try_write(x_object *o, void *buf, int len, x_obj_attr_t *attr)
{
    x_qvhid_t *mctl = (x_qvhid_t *) o;
    int *_dat = (int *)buf;

    printf("Input HID event!!!! {%d:%dx%d}\n",_dat[3],_dat[1],_dat[2]);

    switch (_dat[0])
    {
    case HID_MOUSE_EVENT:
    {
        int v;
        printf("Mouse Event\n");

        /* x */
        v = _dat[1];
        write(mctl->mousepipe, &v, sizeof(int));

        /* y */
        v = _dat[2];
        write(mctl->mousepipe, &v, sizeof(int));

        /* buttons */
        v = _dat[3];
        write(mctl->mousepipe, &v, sizeof(int));

        /* wheel */
        v = (int)0;
        write(mctl->mousepipe, &v, sizeof(int));
    }
        break;

    case HID_KEY_EVENT:
    case HID_SCROLL_EVENT:
    default:
        break;
    };

}

static struct xobjectclass xqhid_class;

static void
___qvhid_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *ob;

    ENTER;

    ob = x_object_add_path(obj, X_IN_DEVDIR "/" HID_VENDOR_ID, NULL);

    EXIT;
}

static struct x_path_listener qvhid_bus_listener;

__x_plugin_visibility
__plugin_init void
x_qvhid_init(void)
{
    ENTER;
    xqhid_class.cname = _XS("hiddemux");
    xqhid_class.psize = (sizeof(x_qvhid_t) - sizeof(x_object));
    xqhid_class.on_assign = &x_qvhid_on_assign;
    xqhid_class.on_append = &x_qvhid_on_append;
    xqhid_class.commit = &x_qvhid_exit;
    xqhid_class.try_write = &x_qvhid_try_write;

    x_class_register_ns(xqhid_class.cname, &xqhid_class, _XS("gobee:media"));

    qvhid_bus_listener.on_x_path_event = &___qvhid_on_x_path_event;
    qvhid_bus_listener.entry.next = NULL;
    qvhid_bus_listener.entry.prev = NULL;

    x_path_listener_add("/", &qvhid_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_qvhid_init)
;

