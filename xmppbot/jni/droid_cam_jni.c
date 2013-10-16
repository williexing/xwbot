/**
 * @file droid_cam_jni.c
 *
 * @date Aug 8, 2012
 * @author artemka
 */

#include <jni.h>
#include <android/log.h>

#undef DEBUG_PRFX
#include <x_config.h>
//#if TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[ xwjni ] "
//#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include "oglrndrr.h"

static char *fbuf = NULL;

JNIEXPORT jint JNICALL
droid_camera_on_setup_frame(JNIEnv *env, jobject jobj, jlong xoid,
    jint w, jint h, jint typ)
{
  int err;
  int sock;
  TRACE("Setting up frame\n");
  fbuf = realloc(fbuf, w * h * 4);
  return 0;
}

JNIEXPORT jint JNICALL
droid_camera_on_new_frame(JNIEnv *env, jobject jobj, jlong xoid,
    jbyteArray buf, jint w, jint h, jint stride)
{
  int sock;
  static int granule;
  char *_fbuf;
  x_object *xobj;

  TRACE("New frame\n");
  if (!xoid)
    return -1;

  if (!buf)
    return -1;

  xobj = X_OBJECT(xoid);

#if 0
  if (!fbuf)
    {
      fbuf =  malloc(w * h * 4);
    }

  (*env)->GetByteArrayRegion(env, buf, 0, w * h * 3, fbuf);
#else
  fbuf = (char *)(*env)->GetPrimitiveArrayCritical(env,buf, 0);
#endif

  // write to channel's cam driver
  _WRITE(xobj, fbuf, w * h * 3, NULL);

#if 0
  (*env)->DeleteLocalRef(env, buf);
#else
  (*env)->ReleasePrimitiveArrayCritical(env,buf, fbuf, 0);
#endif

  return 0;
}
