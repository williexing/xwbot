#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[ xwjni-common ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <plugins/sessions_api/sessions.h>

#include "oglrndrr.h"

static JavaVM *xwJavaVM;
static jobject gCamObject;
static jobject gSysObject;

const char *kCamPath = DROID_CAMERA_CLASS;
const char *kSystemPath = DROID_GOBEESYSTEM_CLASS;
const char *kDisplayPath = DROID_DISPLAY_CLASS;


static x_object *
gobee_bus_init(const char *jid, const char *pw)
{
  x_object * _bus;
  x_obj_attr_t attrs =
    { 0, 0, 0 };

  ENTER;

  /* instantiate '#xbus' object from 'gobee' namespace */
  _bus = _GNEW("#xbus", "gobee");
  BUG_ON(!_bus);

  /* two ways on setting xbus attributes */
  setattr("pword", pw, &attrs);
  setattr("jid", jid, &attrs);
  _ASGN(_bus, &attrs);

  EXIT;
  return _bus;
}

void
gobee_bus_connect(x_object *_bus)
{
  /* start xbus */
  _bus->objclass->finalize(_bus);
}


void
create_display(const char *sid, x_object *xoid)
{
  jclass kls;
  jmethodID mid;
  JNIEnv *envLocal;
  jobject obj;
  jstring jsid;

  TRACE("\n");

  (*xwJavaVM)->GetEnv(xwJavaVM, (void **) &envLocal, JNI_VER);
  kls = (*(*envLocal)->GetObjectClass)(envLocal, gSysObject);
  mid = (*(*envLocal)->GetMethodID)(envLocal, kls,
      "openDisplayDev", "(Ljava/lang/String;J)V");

  jsid = (*envLocal)->NewStringUTF(envLocal, sid);

  (*(*envLocal)->CallVoidMethod)(envLocal, gSysObject, mid,
      jsid, (jlong) (void *) xoid);

  (*(*envLocal)->DeleteLocalRef)(envLocal, jsid);
}


void
create_camera(const char *sid, x_object *xoid)
{
  jclass kls;
  jmethodID mid;
  JNIEnv *envLocal;
  jobject obj;
  jstring jsid;

  TRACE("\n");

  (*xwJavaVM)->GetEnv(xwJavaVM, (void **) &envLocal, JNI_VER);
  kls = (*(*envLocal)->GetObjectClass)(envLocal, gSysObject);
  mid = (*(*envLocal)->GetMethodID)(envLocal, kls,
      "openCameraDev", "(Ljava/lang/String;J)V");

  jsid = (*envLocal)->NewStringUTF(envLocal, sid);
  (*(*envLocal)->CallVoidMethod)(envLocal, gSysObject, mid,
      jsid, (jlong) (void *) xoid);

  (*(*envLocal)->DeleteLocalRef)(envLocal, jsid);
}


int
register_hid_for_Sid(const char *sid, x_object *xoid)
{
  jclass kls;
  jmethodID mid;
  JNIEnv *envLocal;
  jobject obj;
  jstring jsid;

  (*xwJavaVM)->GetEnv(xwJavaVM, (void **) &envLocal, JNI_VER);
  kls = (*(*envLocal)->GetObjectClass)(envLocal, gSysObject);
  mid = (*(*envLocal)->GetMethodID)(envLocal,
      kls, "openHiddev",
      "(Ljava/lang/String;J)V");

  TRACE("Registering new HID device SID(%s),xoid(%p)"
      " kls(%p), mid(%p)\n",sid,xoid,kls,mid);

  jsid = (*envLocal)->NewStringUTF(envLocal, sid);
  (*(*envLocal)->CallVoidMethod)(envLocal, gSysObject,
      mid, jsid, (jlong) (void *) xoid);

  (*(*envLocal)->DeleteLocalRef)(envLocal, jsid);
}

JNIEXPORT jlong
JNICALL
gobee_jni_bus_init(JNIEnv *env, jobject obj, jstring jjid, jstring jpwd)
{
  x_object *bus;
  char *argv[3] =
    { "xmppbot", 0, 0 };

  argv[1] = (*env)->GetStringUTFChars(env, jjid, 0);
  argv[2] = (*env)->GetStringUTFChars(env, jpwd, 0);

  bus = gobee_bus_init(argv[1], argv[2]);

  (*env)->ReleaseStringUTFChars(env, jjid, argv[1]);
  (*env)->ReleaseStringUTFChars(env, jpwd, argv[2]);

  TRACE("INITIALIZED bus object 0x%p\n", bus);

  /* save GobeeMeSystem object */
  gSysObject = obj;

  return (jlong) (void *) bus;
}


JNIEXPORT void
JNICALL
gobee_jni_call_to(JNIEnv *env, jobject obj,
    jlong _lbus, jstring _jid, int audio, int video, int hid)
{
    x_object *_chan_;
    x_object *bus;
    x_object *_sess_ = NULL;
    x_obj_attr_t hints =
    { NULL, NULL, NULL, };
    const char *jid = NULL;

    TRACE("Calling to %s\n",jid);
    jid = (*env)->GetStringUTFChars(env, _jid, 0);
    BUG_ON(!jid);

    bus = X_OBJECT(_lbus);
    BUG_ON(!bus);

    _sess_ = X_OBJECT(x_session_open(
                jid, X_OBJECT(bus), &hints, X_CREAT));

    BUG_ON(!_sess_);

    TRACE("Calling to %s 2\n",jid);

  if (audio)
    {
      /* open/create audio channel */
      _chan_ = X_OBJECT(x_session_channel_open2(
              X_OBJECT(_sess_), "m.A"));

      setattr(_XS("mtype"), _XS("audio"), &hints);
      _ASGN(X_OBJECT(_chan_), &hints);

      attr_list_clear(&hints);
      x_session_channel_set_transport_profile_ns(
          X_OBJECT(_chan_), _XS("__icectl"),
#if 0
          _XS("http://www.google.com/transport/p2p"), &hints);
#else
          _XS("urn:xmpp:jingle:transports:ice-udp:1"), &hints);
#endif
      x_session_channel_set_media_profile_ns(X_OBJECT(_chan_),
          _XS("__rtppldctl"), _XS("jingle"));

      /* add channel payloads */
      setattr("clockrate", "16000", &hints);
      x_session_add_payload_to_channel(_chan_, _XS("110"), _XS("SPEEX"),
          MEDIA_IO_MODE_OUT, &hints);
      /* clean attribute list */
      attr_list_clear(&hints);
    }

  if (video)
    {
     /* open/create video channel */
    _chan_ = x_session_channel_open2(
                X_OBJECT(_sess_), "m.V");

    setattr(_XS("mtype"), _XS("video"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);

    attr_list_clear(&hints);
    x_session_channel_set_transport_profile_ns(
                X_OBJECT(_chan_), _XS("__icectl"),
#if 0
          _XS("http://www.google.com/transport/p2p"), &hints);
#else
      _XS("urn:xmpp:jingle:transports:ice-udp:1"), &hints);
#endif

    x_session_channel_set_media_profile_ns(
                X_OBJECT(_chan_),
                _XS("__rtppldctl"),_XS("jingle"));

    /* add channel payloads */
    attr_list_clear(&hints);
    setattr(_XS("clockrate"), _XS("90000"), &hints);
    setattr(_XS("width"), _XS("320"), &hints);
    setattr(_XS("height"), _XS("240"), &hints);
    setattr(_XS("sampling"), _XS("YCbCr-4:2:0"), &hints);

#if 0
        x_session_add_payload_to_channel(
                    X_OBJECT(_chan_),
                    _XS("96"), _XS("THEORA"),
                    MEDIA_IO_MODE_OUT, &hints);
#else
        x_session_add_payload_to_channel(
                    X_OBJECT(_chan_),
                    _XS("98"), _XS("VP8"),
                    MEDIA_IO_MODE_OUT, &hints);
#endif

    attr_list_clear(&hints);
    if (hid)
      {
        x_session_add_payload_to_channel(
            X_OBJECT(_chan_), _XS("101"),
            _XS("HID"), MEDIA_IO_MODE_OUT, &hints);

        attr_list_clear(&hints);
      }
    }

#if 0
if (hid)
  {
    /* open/create hid channel */
    _chan_ = x_session_channel_open2(
                X_OBJECT(_sess_), "m.H");

    setattr(_XS("mtype"), _XS("application"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);

    attr_list_clear(&hints);
    x_session_channel_set_transport_profile_ns(
                X_OBJECT(_chan_), _XS("__icectl"),
                _XS("jingle"), &hints);
    x_session_channel_set_media_profile_ns(
                X_OBJECT(_chan_), _XS("__rtppldctl"),_XS("jingle"));
    x_session_add_payload_to_channel(
                X_OBJECT(_chan_), _XS("101"), _XS("HID"), MEDIA_IO_MODE_OUT, &hints);
  }
#endif

    // commit channel
    setattr(_XS("$commit"), _XS("yes"), &hints);
    _ASGN(X_OBJECT(_chan_), &hints);
    attr_list_clear(&hints);

    x_session_start(X_OBJECT(_sess_));

}




static x_object *cached_ips = NULL;

static void
___gobee_on_networking_path_event(void *_obj)
{
  x_object *tmpo;
  x_object *netdir;
  x_object *ip;

  ENTER;

  if (_obj)
    {
      netdir = _CHLD(X_OBJECT(_obj), _XS("$networking"));
      if (!netdir)
        {
          netdir = _GNEW(_XS("$message"),NULL);
          _SETNM(netdir, _XS("$networking"));
          x_object_append_child_no_cb(X_OBJECT(_obj), netdir);
        }

      for (tmpo = _CHLD(cached_ips,_XS("$interface")); tmpo; tmpo = _NXT(tmpo))
        {
          ip = _CPY(tmpo);

          TRACE("Inserting %s into %s\n", _AGET(ip,"ip"), _GETNM(netdir));

          x_object_append_child_no_cb(netdir, ip);
        }
    }

  EXIT;
}

static struct x_path_listener networking_bus_listener;

__x_plugin_visibility
__plugin_init void
gobee_on_networking_init(void)
{
  cached_ips = _GNEW("$message",NULL);
  networking_bus_listener.on_x_path_event = &___gobee_on_networking_path_event;
  x_path_listener_add("__proc", &networking_bus_listener);
  return;
}

PLUGIN_INIT(gobee_on_networking_init)
;

/*
 * Class:     com_xw_xbus_XBus
 * Method:    xw_connect
 * Signature: (J)J
 */
JNIEXPORT void
JNICALL
gobee_jni_bus_add_ip(JNIEnv *env, jobject obj, jlong _lbus, jstring _ip)
{
  x_object *bus;
  x_object *tmp,*tmp2;
  x_object *procdir;
  x_object *netdir;
  char *ip;
  ENTER;

  bus = X_OBJECT(_lbus);

  ip = (*env)->GetStringUTFChars(env, _ip, 0);
  TRACE("ADDING %s address\n", ip);

  /* dataport candidate */
  tmp = _GNEW(_XS("$interface"),NULL);
  BUG_ON(!tmp);

  TRACE("ADDING IP ADDRESS '%s'\n", ip);
  _ASET(tmp, _XS("ip"), ip);
  _ASET(tmp, _XS("type"), _XS("IPv4"));

  _INS(cached_ips, tmp);

  procdir = _CHLD(bus, _XS("__proc"));
  if (!procdir)
    goto add_ip_exit;

  netdir = _CHLD(procdir, _XS("$networking"));
  if (!netdir)
    goto add_ip_exit;

  tmp2 = _CPY(tmp);
  x_object_append_child_no_cb(netdir, tmp2);

  add_ip_exit:
  EXIT;
}

/*
 * Class:     com_xw_xbus_XBus
 * Method:    xw_connect
 * Signature: (J)J
 */
JNIEXPORT void
JNICALL
gobee_jni_bus_connect(JNIEnv *env, jobject obj, jlong _lbus)
{
  x_object *bus;
  ENTER;
  bus = (x_object *) (void *) _lbus;
  gobee_bus_connect(bus);
  EXIT;
}

static int
org_xw_register_natives(JNIEnv *env)
{
  jclass kls;
  JNINativeMethod methods[] =
    {
      { "_n_setup_frame", "(JIII)I", (void *) &droid_camera_on_setup_frame },
      { "_n_push_frame", "(J[BIII)I", (void *) &droid_camera_on_new_frame }, };

  JNINativeMethod methods2[] =
    {
      { "jni_bus_init", "(Ljava/lang/String;Ljava/lang/String;)J",(void *) &gobee_jni_bus_init },
      { "jni_bus_connect", "(J)V", (void *) &gobee_jni_bus_connect },
      { "jni_bus_add_ip", "(JLjava/lang/String;)V",(void *) &gobee_jni_bus_add_ip },
      { "jni_call_to", "(JLjava/lang/String;III)V",(void *) &gobee_jni_call_to },
    };

  JNINativeMethod methods3[] =
    {
      { "_n_emit_render_frame", "(J[III)V", (void *) &n_emit_render_frame },
      { "_n_emit_setup_display", "(JII)V", (void *) &n_emit_setup_display },
      { "_n_emit_new_display", "(J)V", (void *) &n_emit_new_display },
      { "_n_emit_hid_event", "(JIIII)V", (void *) &n_emit_hid_event },
    };

  kls = (*(*env)->FindClass)(env, DROID_CAMERA_CLASS);
  if ((*(*env)->RegisterNatives)(env, kls, methods, 2) != JNI_OK)
    {
      TRACE("Failed to register native methods\n");
    }

  kls = (*(*env)->FindClass)(env, DROID_GOBEESYSTEM_CLASS);
  if ((*(*env)->RegisterNatives)(env, kls, methods2, 4) != JNI_OK)
    {
      TRACE("Failed to register native methods\n");
    }

  kls = (*(*env)->FindClass)(env, DROID_DISPLAY_CLASS);
  if ((*(*env)->RegisterNatives)(env, kls, methods3, 4) != JNI_OK)
    {
      TRACE("Failed to register native methods\n");
    }

  return 0;
}


int
__droid_on_presence_msg_rx(x_object *to, const x_object *from,
    const x_object *o)
{
  jclass kls;
  jmethodID mid;
  JNIEnv *envLocal;
  jobject obj;
  jstring jid;
  jstring resource;

  x_string_t presj = _AGET(o,"jid");

  if (!presj)
    return 0;

  TRACE("New message\n");

  (*xwJavaVM)->GetEnv(xwJavaVM, (void **) &envLocal, JNI_VER);

  TRACE("OK1, (*(*%p)->GetObjectClass)(%p, %p);\n",
      envLocal, envLocal, gSysObject);

 kls = (*(*envLocal)->GetObjectClass)(envLocal, gSysObject);
 TRACE("OK1\n");
mid = (*(*envLocal)->GetMethodID)(envLocal, kls,
      "proxyOnContactPresence", "(Ljava/lang/String;Ljava/lang/String;)V");

TRACE("OK1\n");
  jid = (*envLocal)->NewStringUTF(envLocal, presj);
  TRACE("OK1\n");
  resource = (*envLocal)->NewStringUTF(envLocal, "fixme!");

  TRACE("OK1\n");
  (*(*envLocal)->CallVoidMethod)(envLocal, gSysObject, mid,
      jid,resource);

  TRACE("OK1\n");
  (*(*envLocal)->DeleteLocalRef)(envLocal, jid);
  TRACE("OK2\n");
 (*(*envLocal)->DeleteLocalRef)(envLocal, resource);
 TRACE("OK3\n");

 return 0;
}

static struct xobjectclass jni_hub_class;

static void
___droid_presence_x_path_event(void *_obj)
{
  x_object *obj = X_OBJECT(_obj);
  x_object *ob;
  x_object *lstnr;
  x_object *parent;

  ENTER;

#if 0
  parent = _PARNT(obj);
  if (!parent ||
      !EQ(_GETNM(parent),"__proc"))
    return;
#endif

//  if (EQ(_GETNM(obj),"__proc"))
//    {
  ob = x_object_mkpath(X_OBJECT(obj->bus), "__proc/android");
  lstnr = _CHLD(ob,jni_hub_class.cname);
  if (!lstnr)
    {
      lstnr = _GNEW(jni_hub_class.cname,jni_hub_class.ns);
      _INS(ob, lstnr);
    }
//    }

  _SBSCRB(obj,lstnr);

  EXIT;
}

static struct x_path_listener droid_presence_listener;
//static struct x_path_listener droid_proc_listener;

__x_plugin_visibility
__plugin_init void
x_presence_listener_init(void)
{
  ENTER;

  jni_hub_class.cname = _XS("jnihub");
  jni_hub_class.psize = 0;
  jni_hub_class.rx = &__droid_on_presence_msg_rx;
  jni_hub_class.ns = _XS("gobee:android");

  x_class_register_ns(jni_hub_class.cname, &jni_hub_class,
      jni_hub_class.ns);

  droid_presence_listener.on_x_path_event = &___droid_presence_x_path_event;
//  droid_proc_listener.on_x_path_event = &___droid_cam_on_x_path_event;

  x_path_listener_add("presence", &droid_presence_listener);
//  x_path_listener_add("__proc", &droid_proc_listener);

  EXIT;
  return;
}

jint
JNI_OnLoad(JavaVM* vm, void* reserved)
{
  JNIEnv *env;

  TRACE("ENTER\n");

  /* Cached VM */
  xwJavaVM = vm;

  if ((*vm)->GetEnv(vm, (void**) &env, JNI_VER) != JNI_OK)
    {
      TRACE("Failed to get the environment using GetEnv()\n");
      return -1;
    }

//<Class instance caching>
  org_xw_register_natives(env);

  return JNI_VER;

}
