/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_ALSA_SOUND_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-mic] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <dlfcn.h>
#include <alsa/asoundlib.h>

#define X_IN_DEVDIR "__proc/in"
#define MICROPHONE_VENDOR_ID "gobee_Microphone"

typedef struct x_vmic
{
    x_object xobj;
    struct ev_periodic mic_qwartz;
    unsigned char *framebuf;
    int sample_rate;
    int sample_size;
    snd_pcm_t *alsahandle;
} x_vmic_t;

#define ASOUND_LIB_NAME "libasound.so"

static struct {
    int loaded;
    int load_count;
    void *dlhandle;

    int (*_pcm_open)(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
    int (*_pcm_hw_params_any)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
    int (*_pcm_hw_params_set_access)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
    int (*_pcm_hw_params_set_format)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);

    int (*_pcm_hw_params_set_rate)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir);
    int (*_pcm_hw_params_set_channels)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);

    int (*_pcm_hw_params_set_buffer_size)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val);
    int (*_pcm_hw_params)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);

    int (*_pcm_prepare)(snd_pcm_t *pcm);
    int (*_pcm_close)(snd_pcm_t *pcm);
    snd_pcm_sframes_t (*_pcm_readi)(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size);

    const char *(*_strerror)(int errnum);
    int (*_pcm_recover)(snd_pcm_t *pcm, int err, int silent);
} alsa_wrapper;

static int
load_alsa_lib(void /*const char *libname*/)
{
    x_memset(&alsa_wrapper,0,sizeof(alsa_wrapper));
    alsa_wrapper.dlhandle = dlopen(ASOUND_LIB_NAME, RTLD_GLOBAL | RTLD_NOW);

    alsa_wrapper._pcm_open = dlsym(alsa_wrapper.dlhandle, "snd_pcm_open");
    alsa_wrapper._pcm_hw_params = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params");
    alsa_wrapper._pcm_hw_params_any = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_any");
    alsa_wrapper._pcm_hw_params_set_access = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_access");
    alsa_wrapper._pcm_hw_params_set_buffer_size = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_buffer_size");
    alsa_wrapper._pcm_hw_params_set_channels = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_channels");
    alsa_wrapper._pcm_hw_params_set_format = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_format");
    alsa_wrapper._pcm_hw_params_set_rate = dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_rate");
    alsa_wrapper._pcm_close = dlsym(alsa_wrapper.dlhandle, "snd_pcm_close");
    alsa_wrapper._pcm_prepare = dlsym(alsa_wrapper.dlhandle, "snd_pcm_prepare");
    alsa_wrapper._pcm_readi = dlsym(alsa_wrapper.dlhandle, "snd_pcm_readi");
    alsa_wrapper._pcm_recover = dlsym(alsa_wrapper.dlhandle, "snd_pcm_recover");
    alsa_wrapper._strerror = dlsym(alsa_wrapper.dlhandle, "snd_strerror");

    TRACE("ALSA lib loaded! %p %p %p\n",
          alsa_wrapper.dlhandle,
          alsa_wrapper._pcm_open,
          alsa_wrapper._pcm_readi
          );
}

#define ALSA_BRATE 16000
static int
open_alsa_dev(x_vmic_t *thiz)
{
    int rc;
    unsigned int rate;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t soundbufsize;

    int err = -1;
    if ((err = alsa_wrapper._pcm_open (&thiz->alsahandle,
                             "default", SND_PCM_STREAM_CAPTURE,
                             SND_PCM_NONBLOCK | SND_PCM_ASYNC)) < 0) {
        TRACE("cannot open audio device (%s)\n",
              snd_strerror (err));
    }

    snd_pcm_hw_params_alloca(&params);
    alsa_wrapper._pcm_hw_params_any(thiz->alsahandle, params);
    alsa_wrapper._pcm_hw_params_set_access(thiz->alsahandle, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    alsa_wrapper._pcm_hw_params_set_format(thiz->alsahandle, params,
                                 SND_PCM_FORMAT_S16_LE);

    rate = ALSA_BRATE;
    alsa_wrapper._pcm_hw_params_set_rate(thiz->alsahandle, params, rate, 0);
    alsa_wrapper._pcm_hw_params_set_channels(thiz->alsahandle, params, 1);

    soundbufsize = 320; /* uframes */
    alsa_wrapper._pcm_hw_params_set_buffer_size(thiz->alsahandle, params, soundbufsize);

    rc = alsa_wrapper._pcm_hw_params(thiz->alsahandle, params);
    if (rc < 0)
    {
        TRACE("unable to set hw parameters: %s\n", snd_strerror(rc));
    }
    else
    {
        TRACE("Hw parameters were set\n");
    }

    alsa_wrapper._pcm_prepare(thiz->alsahandle);

    return err;
}

static void
release_alsa_dev(x_vmic_t *thiz)
{
    alsa_wrapper._pcm_close (thiz->alsahandle);
}


static int
__xmic_check_writehandler(x_vmic_t *thiz)
{
    x_object *tmp;
    x_object *parent = _PARNT(X_OBJECT(thiz));

    if (!thiz->xobj.write_handler)
    {
        if (parent)
            tmp = _CHLD(parent, "out$media");

        if (!tmp)
        {

            TRACE("No output channel... Decreasing mic status...\n");
            ERROR;
            return -1;
        }

        tmp = _CHLD(tmp,NULL);
        if (!tmp)
        {
            TRACE("No output channel... Decreasing mic status...\n");
            ERROR;
            return -1;
        }
        else
        {
            x_object_set_write_handler(X_OBJECT(&thiz->xobj),X_OBJECT(tmp));
        }
    }
    else
    {
        TRACE("Handler OK\n");
    }

    return 0;
}

static void
__xmic_periodic_cb(xevent_dom_t *loop, struct ev_periodic *w, int revents)
{
    int len;
    x_vmic_t *thiz = w->data;

    TRACE("\n");

    if (!thiz || !thiz->framebuf
            || __xmic_check_writehandler(thiz) != 0
            )
    {
        TRACE("ERROR State!\n");
        return;
    }
    else
    {
        TRACE("Ok! Write handler = '%s'\n", _GETNM(thiz->xobj.write_handler));
    }

    TRACE("\n");

    if ((len = alsa_wrapper._pcm_readi (thiz->alsahandle, thiz->framebuf, 320)) < 0) {
        TRACE("read from audio interface failed (%s)\n",
              alsa_wrapper._strerror (len));
//        x_object_remove_watcher(thiz, &thiz->mic_qwartz, EVT_PERIODIC);
        alsa_wrapper._pcm_recover(thiz->alsahandle,len,1);
        return;
    }
    else
    {

        // dump
        #ifdef DEBUG_DATA
        {
    //        int fd = x_open("sound-near.pcm", O_APPEND | O_RDWR);
    //        x_write(fd, thiz->framebuf, len * 2);
    //        x_close(fd);
        }
        #endif
        TRACE("Frames read from alsa = %d\n", len);

        {
            int i;
            for (i = 0;  i < len / 320; i++)
              _WRITE(thiz->xobj.write_handler, thiz->framebuf + (i*640), 640, NULL);
        }
    }

    TRACE("\n");
}


static int
x_mic_on_append(x_object *so, x_object *parent)
{
    x_vmic_t *thiz = (x_vmic_t *) so;
    ENTER;

    TRACE("\n");

    thiz->framebuf = x_malloc (1024);

    thiz->mic_qwartz.data = (void *) so;

    /** @todo This should be moved in x_object API */
    ev_periodic_init(&thiz->mic_qwartz, &__xmic_periodic_cb, 0., 0.020, 0);

    x_object_add_watcher(so, &thiz->mic_qwartz, EVT_PERIODIC);

    _ASET(so,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
    _ASET(so,_XS("Descr"),_XS("Micriphone"));

    open_alsa_dev(thiz);

    EXIT;
    return 0;
}

static int
x_mic_on_remove(x_object *so, x_object *parent)
{
    x_vmic_t *thiz = (x_vmic_t *) so;
    ENTER;

    TRACE("\n");

    x_object_remove_watcher(so, &thiz->mic_qwartz, EVT_PERIODIC);

    //    /** @todo This should be moved in x_object API */
    //    ev_periodic_init(&thiz->mic_qwartz, &__xmic_periodic_cb, 0., 0.020, 0);

    if(thiz->framebuf)
    {
        x_free(thiz->framebuf);
        thiz->framebuf = NULL;
    }

    thiz->mic_qwartz.data = NULL;
    release_alsa_dev(thiz);

    EXIT;
    return 0;
}

static x_object *
x_mic_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
    x_vmic_t *mctl = (x_vmic_t *) this__;
    ENTER;

    x_object_default_assign_cb(this__, attrs);

    EXIT;
    return this__;
}

static void
x_mic_exit(x_object *so)
{
    x_vmic_t *thiz = (x_vmic_t *) so;
    ENTER;

    TRACE();

    //    release_alsa_dev(thiz);

    if (thiz->xobj.write_handler)
    {
        _REFPUT(thiz->xobj.write_handler, NULL);
        thiz->xobj.write_handler = NULL;
    }

    EXIT;
}

static struct xobjectclass xmic_class;

static void
___mic_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *ob;

    ENTER;

    ob = x_object_add_path(obj, X_IN_DEVDIR "/" MICROPHONE_VENDOR_ID, NULL);

    EXIT;
}

static struct x_path_listener mic_bus_listener;

__x_plugin_visibility
__plugin_init void
x_mic_init(void)
{
    ENTER;

    load_alsa_lib();

    xmic_class.cname = _XS("microphone");
    xmic_class.psize = (sizeof(x_vmic_t) - sizeof(x_object));
    xmic_class.on_assign = &x_mic_on_assign;
    xmic_class.on_append = &x_mic_on_append;
    xmic_class.commit = &x_mic_exit;
    xmic_class.on_remove = &x_mic_on_remove;

    x_class_register_ns(xmic_class.cname, &xmic_class, _XS("gobee:media"));

    mic_bus_listener.on_x_path_event = &___mic_on_x_path_event;

    /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
    x_path_listener_add("/", &mic_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_mic_init)
;

