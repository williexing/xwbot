/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-DISPLAY] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <gtk/gtk.h>
//#include <gobject/gobject.h>
#include <gdk/gdk.h>
#include <gdk/gdkpixbuf.h>
#include <glade/glade.h>

#include <theora/theora.h>
#include <theora/theoradec.h>
#include <theora/theoraenc.h>

int
xgl_start(GtkWidget *widget, void *data);
int
xgl_render_scene(GtkWidget *widget, GdkEvent *_event, void *data);
int
xgl_configure(GtkWidget *widget, GdkEvent *ev, void *data);
int
Render(th_ycbcr_buffer yuv);

void
worker_push(void
(*func)(void *data), void *data);

#define X_OUT_DEVDIR "__proc/out"
#define DISPLAY_VENDOR_ID "Display_GLESv2"

typedef struct virtual_display
{
  x_object xobj;
  GtkWidget *window;
  GtkWidget *gl_win;
} virtual_display_t;

static void
__invalidate(void *data)
{
  GtkWidget *widget = (GtkWidget *) data;
  gdk_window_invalidate_rect(widget->window, &widget->allocation, FALSE);
//  gdk_window_process_updates(widget->window, TRUE);
}

static int
x_vdisplay_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
  int err;
  th_ycbcr_buffer yuv;
  virtual_display_t *mctl = (virtual_display_t *) o;
  u_int32_t l;

  yuv[0].data = buf;
  yuv[0].width = 320;
  yuv[0].height = 240;
  yuv[0].stride = 352;

  yuv[1].width = yuv[0].width >> 1;
  yuv[1].height = yuv[0].height >> 1;
  yuv[1].stride = yuv[0].stride >> 1;

  yuv[2].width = yuv[1].width;
  yuv[2].height = yuv[1].height;
  yuv[2].stride = yuv[1].stride;

  yuv[1].data = yuv[0].data + yuv[0].stride * yuv[0].height;
  yuv[2].data = yuv[1].data + yuv[1].width * yuv[1].height;

  Render(yuv);
  worker_push(&__invalidate, mctl->window);

  return len;
}

static int
x_vdisplay_try_writev(x_object *o, x_iobuf_t *iov, u_int32_t iovlen, x_obj_attr_t *attr)
{
  int err;
  th_ycbcr_buffer yuv;
  virtual_display_t *mctl = (virtual_display_t *) o;
  u_int32_t l;

  yuv[0].data = X_IOV_DATA(&iov[0]);
  yuv[0].width = 320;
  yuv[0].height = 240;
  yuv[0].stride = 352;

  yuv[1].width = yuv[0].width >> 1;
  yuv[1].height = yuv[0].height >> 1;
  yuv[1].stride = yuv[0].stride >> 1;

  yuv[2].width = yuv[1].width;
  yuv[2].height = yuv[1].height;
  yuv[2].stride = yuv[1].stride;

  yuv[1].data = X_IOV_DATA(&iov[1]);
  yuv[2].data = X_IOV_DATA(&iov[2]);

  Render(yuv);
  worker_push(&__invalidate, mctl->window);

  return 100;
}


/**
 * This should be run in UI thread.
 */
static void
__x_vdisplay_show(void *so)
{
  GladeXML *xml;
  virtual_display_t *mctl = (virtual_display_t *) so;
  ENTER;

  /* load the interface */
  xml = glade_xml_new(/* SHARE_DIR */"xwrcptex.glade", NULL, NULL);

  TRACE("\n");

  /* connect the signals in the interface */
//  glade_xml_signal_autoconnect(xml);
  mctl->window = glade_xml_get_widget(xml, "mainwin");
//  mctl->gl_win = glade_xml_get_widget(xml, "mainwin");

  TRACE("\n");

  g_signal_connect(G_OBJECT(mctl->window), "expose_event",
      G_CALLBACK (xgl_render_scene), NULL);

  TRACE("\n");
  g_signal_connect(G_OBJECT(mctl->window), "map_event", G_CALLBACK (xgl_start),
      NULL);

  TRACE("\n");
  g_signal_connect(G_OBJECT(mctl->window), "configure_event",
      G_CALLBACK (xgl_configure), NULL);

  TRACE("\n");
  EXIT;
  return;
}

static int
x_vdisplay_on_append(x_object *so, x_object *parent)
{
  GladeXML *xml;
  virtual_display_t *mctl = (virtual_display_t *) so;
  ENTER;

  /* push worker */
  worker_push(&__x_vdisplay_show, (void *) so);

  EXIT;
  return 0;
}

static x_object *
x_vdisplay_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  virtual_display_t *mctl = (virtual_display_t *) this__;
  ENTER;

  x_object_default_assign_cb(this__, attrs);

  EXIT;
  return this__;
}

static void
x_vdisplay_exit(x_object *this__)
{
  virtual_display_t *mctl = (virtual_display_t *) this__;
  ENTER;
//  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
  EXIT;
}

static struct xobjectclass virt_display_classs;

static void
___vdisplay_on_x_path_event(void *_obj)
{
  x_object *obj = _obj;
  x_object *ob;

  ENTER;

  ob = x_object_add_path(obj, X_OUT_DEVDIR "/" DISPLAY_VENDOR_ID, NULL);

  EXIT;
}

static struct x_path_listener virtdisplay_bus_listener;

__x_plugin_visibility
__plugin_init void
x_virtdisplay_init(void)
{
  ENTER;
  virt_display_classs.cname = _XS("video_player");
  virt_display_classs.psize = (sizeof(virtual_display_t) - sizeof(x_object));
//  virt_display_classs.match = &x_vdisplay_match;
  virt_display_classs.on_assign = &x_vdisplay_on_assign;
  virt_display_classs.on_append = &x_vdisplay_on_append;
  virt_display_classs.finalize = &x_vdisplay_exit;
  virt_display_classs.try_write = &x_vdisplay_try_write;
  virt_display_classs.try_writev = &x_vdisplay_try_writev;

  x_class_register_ns(virt_display_classs.cname, &virt_display_classs,
      _XS("gobee:media"));

  virtdisplay_bus_listener.on_x_path_event = &___vdisplay_on_x_path_event;

  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &virtdisplay_bus_listener);
  EXIT;
  return;
}

PLUGIN_INIT(x_virtdisplay_init)
;

