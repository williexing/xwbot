/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#undef DEBUG_PRFX
#include <x_config.h>
#if 1 //TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[ xwjni-display ] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include "oglrndrr.h"

int Render(virtual_display_t *d, img_plane_buffer *yuv);
int __gl_Render(virtual_display_t *d);


void create_display(const char *sid, x_object *xoid);

#define X_OUT_DEVDIR "__proc/out"
#define DISPLAY_VENDOR_ID "Display_GLESv2"

typedef struct vdisplay_gles_ctx
{
  int32_t _id;
  GLuint _textureIds[3]; // Texture id of Y,U and V texture.
  GLuint _program;
  GLuint _vPositionHandle;
  GLsizei _textureWidth;
  GLsizei _textureHeight;
  int viewportWidth;
  int viewportHeight;

  /** yuv frame buffer */
  img_plane_buffer ycbcr;
  img_plane_buffer *YUVbuf;

  GLfloat _vertices[20];
  const char g_indices[6];

  GLuint program;
} vdisplay_gles_ctx_t;

JNIEXPORT void
JNICALL
n_emit_new_display(JNIEnv *env, jobject obj, jlong xoid)
{
  virtual_display_t *d = (virtual_display_t*) X_OBJECT(xoid);
  TRACE("\n");
  if (!xoid)
    return;
  d->jobj = obj;
  d->env = env;
  TRACE("\n");
}

JNIEXPORT void
JNICALL
n_emit_setup_display(JNIEnv *env, jobject obj, jlong xoid, jint w, jint h)
{
  virtual_display_t *d = (virtual_display_t*) X_OBJECT(xoid);
 TRACE("DROID-DISPLAY\n");
  x_gles2_init(d, w, h);
}

/**
 * This called from java GL to update scrren
 */
JNIEXPORT void
JNICALL
n_emit_render_frame(JNIEnv *env, jobject jobj, jlong xoid, jintArray buf,
    jint w, jint h)
{
  virtual_display_t *d = (virtual_display_t*) X_OBJECT(xoid);
//    __android_log_print(ANDROID_LOG_WARN, "[DROID-DISPLAY] warm",
//        "%s: ENTER\n", __FUNCTION__);
//    __android_log_print(ANDROID_LOG_WARN, "[DROID-DISPLAY] warm 2",
//        "%s: ENTER\n", __FUNCTION__, __LINE__);
 __gl_Render(d);
 return;
}

void
x_vdisplay_java_notify_geometry(virtual_display_t *d,
    int v1, int v2, int v3, int v4, int v5, int v6)
{
  jclass kls;
  jmethodID mid;
  JNIEnv *envLocal;

  TRACE("\n");

  if (!d || !d->jobj || !d->env)
    return;

  envLocal = d->env;

  kls = (*(*envLocal)->GetObjectClass)(envLocal, d->jobj);
  mid = (*(*envLocal)->GetMethodID)(envLocal, kls, "setViewport", "(IIIIII)V");

  (*(*envLocal)->CallVoidMethod)(envLocal, d->jobj, mid,
      v1,v2,v3,v4,v5,v6);

}

/*------------------------------------*/

static int
x_vdisplay_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
  int err;
  img_plane_buffer yuv;
  virtual_display_t *mctl = (virtual_display_t *) o;
  u_int32_t l;

  TRACE("\n");
  yuv[0].data = buf;
  yuv[0].width = mctl->frameWidth;
  yuv[0].height = mctl->frameHeight;
  yuv[0].stride = mctl->frameStride;

  yuv[1].width = yuv[0].width / 2;
  yuv[1].height = yuv[0].height / 2;
  yuv[1].stride = yuv[0].stride / 2;

  yuv[2].width = yuv[1].width;
  yuv[2].height = yuv[1].height;
  yuv[2].stride = yuv[1].stride;

  yuv[1].data = yuv[0].data + yuv[0].stride * yuv[0].height;
  yuv[2].data = yuv[1].data + yuv[1].width * yuv[1].height;

  Render(mctl, yuv);

  TRACE("\n");

  return len;
}

static int
x_vdisplay_try_writev(x_object *o, x_iobuf_t *iov, u_int32_t iovlen, x_obj_attr_t *attr)
{
  int err;
  img_plane_buffer yuv;
  virtual_display_t *mctl = (virtual_display_t *) o;
  u_int32_t l;
  int *stride = NULL;

  ENTER;

  yuv[0].data = X_IOV_DATA(&iov[0]);
  yuv[0].width = mctl->frameWidth;
  yuv[0].height = mctl->frameHeight;
  yuv[0].stride = mctl->frameStride;

  yuv[1].width = yuv[0].width / 2;
  yuv[1].height = yuv[0].height / 2;
  yuv[1].stride = yuv[0].stride / 2;

  yuv[2].width = yuv[1].width;
  yuv[2].height = yuv[1].height;
  yuv[2].stride = yuv[1].stride;

  yuv[1].data = X_IOV_DATA(&iov[1]);
  yuv[2].data = X_IOV_DATA(&iov[2]);

  TRACE("Render...\n");
  Render(mctl, yuv);

  return 100;
}

static int
x_vdisplay_on_append(x_object *so, x_object *parent)
{
  virtual_display_t *mctl = (virtual_display_t *) so;
  ENTER;

  /* push worker to be run in UI thread */
  TRACE("\n");
//  worker_push(&__x_vdisplay_show, (void *) so);
  create_display(_ENV(so,"sid"), so);
  mctl->frameWidth = 320;
  mctl->frameHeight = 240;
  mctl->frameStride = 320;
  EXIT;
  return 0;
}

static x_object *
x_vdisplay_on_assign(x_object *thiz, x_obj_attr_t *attrs)
{
  register x_obj_attr_t *attr;
  int update = 0;
  virtual_display_t *mctl = (virtual_display_t *) thiz;

  ENTER;

  for (attr = attrs->next; attr; attr = attr->next)
  {
      if (EQ("width", attr->key))
      {
          mctl->frameWidth = atoi(attr->val);
          TRACE("GobeeDisplay::assign(): width=%d\n",mctl->frameWidth);
          update = 1;
      }
      else if (EQ("stride", attr->key))
      {
          mctl->frameStride = atoi(attr->val);
          TRACE("GobeeDisplay::assign(): stride=%d\n",mctl->frameStride);
          update = 1;
      }
      else  if (EQ("height", attr->key))
      {
          mctl->frameHeight = atoi(attr->val);
          TRACE("GobeeDisplay::assign(): height=%d\n",mctl->frameHeight);
          update = 1;
      }

      _ASET(thiz, attr->key, attr->val);
  }

  if (update)
  {
      // this->framebuf update
  }

  EXIT;
  return thiz;
}

static void
x_vdisplay_exit(x_object *this__)
{
  virtual_display_t *mctl = (virtual_display_t *) this__;
  ENTER;
  TRACE("\n");
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
//  virt_display_classs.try_write = &x_vdisplay_try_write;
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
