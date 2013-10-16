
#include <x_config.h>

#define DEBUG_PRFX "[ V-SOUND ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <stdio.h>
#include <dlfcn.h>
#include <alsa/asoundlib.h>

#define X_OUT_DEVDIR "__proc/out"
#define AUDIO_VENDOR_ID "QtSound_ALSA"

#include "vspeaker.h"



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
    snd_pcm_sframes_t (*_pcm_writei)(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
    const char *(*_strerror)(int errnum);
    int (*_pcm_recover)(snd_pcm_t *pcm, int err, int silent);
} alsa_wrapper;

static int
load_alsa_lib(void /*const char *libname*/)
{
    x_memset(&alsa_wrapper,0,sizeof(alsa_wrapper));
    alsa_wrapper.dlhandle = dlopen(ASOUND_LIB_NAME, RTLD_GLOBAL | RTLD_NOW);

    alsa_wrapper._pcm_open = (int (*)(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_open");

    alsa_wrapper._pcm_hw_params = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params");

    alsa_wrapper._pcm_hw_params_any = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_any");

    alsa_wrapper._pcm_hw_params_set_access = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_access");

    alsa_wrapper._pcm_hw_params_set_buffer_size = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t val))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_buffer_size");

    alsa_wrapper._pcm_hw_params_set_channels = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_channels");

    alsa_wrapper._pcm_hw_params_set_format = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_format");

    alsa_wrapper._pcm_hw_params_set_rate = (int (*)(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_hw_params_set_rate");

    alsa_wrapper._pcm_close = (int (*)(snd_pcm_t *pcm))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_close");

    alsa_wrapper._pcm_prepare = (int (*)(snd_pcm_t *pcm))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_prepare");

    alsa_wrapper._pcm_readi = (snd_pcm_sframes_t (*)(snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_readi");

    alsa_wrapper._pcm_writei = (snd_pcm_sframes_t (*)(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_writei");

    alsa_wrapper._pcm_recover = (int (*)(snd_pcm_t *pcm, int err, int silent))
            dlsym(alsa_wrapper.dlhandle, "snd_pcm_recover");

    alsa_wrapper._strerror = (const char *(*)(int errnum))
            dlsym(alsa_wrapper.dlhandle, "snd_strerror");

    TRACE("ALSA lib loaded! %p %p %p\n",
          alsa_wrapper.dlhandle,
          alsa_wrapper._pcm_open,
          alsa_wrapper._pcm_readi
          );
}


#define ALSA_BRATE 16000

#ifdef LINUX
int
GobeeSpeaker::alsa_open()
{
    int rc;
    unsigned int rate;
    int dir;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t soundbufsize;

    ENTER;

    /* Open PCM device for playback. */
    rc = alsa_wrapper._pcm_open(&this->snddev, "default",
                      SND_PCM_STREAM_PLAYBACK,
                      SND_PCM_NONBLOCK | SND_PCM_ASYNC
                      );
    if (rc < 0)
    {
        TRACE("unable to open pcm device: %s\n", alsa_wrapper._strerror(rc));
        exit(1);
    }

    snd_pcm_hw_params_alloca(&params);
    alsa_wrapper._pcm_hw_params_any(this->snddev, params);
    alsa_wrapper._pcm_hw_params_set_access(this->snddev, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    alsa_wrapper._pcm_hw_params_set_format(this->snddev, params, SND_PCM_FORMAT_S16_LE);

    rate = ALSA_BRATE;
    alsa_wrapper._pcm_hw_params_set_rate(this->snddev, params, rate, 0);
    alsa_wrapper._pcm_hw_params_set_channels(this->snddev, params, 1);

    soundbufsize = 640;
    alsa_wrapper._pcm_hw_params_set_buffer_size(this->snddev, params, soundbufsize);

    rc = alsa_wrapper._pcm_hw_params(this->snddev, params);
    if (rc < 0)
    {
        TRACE("unable to set hw parameters: %s\n", snd_strerror(rc));
        return rc;
    }
    else
    {
        TRACE("Hw parameters were set\n");
    }

    alsa_wrapper._pcm_prepare(this->snddev);

    EXIT;
}
#endif


int
GobeeSpeaker::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    snd_pcm_sframes_t avail;
    unsigned char *_buf = (unsigned char *)buf;
    unsigned char *_end = _buf + len;

    ENTER;
#ifdef LINUX
    if (!this->state)
        return -1;

    if (!this->snddev)
    {
        alsa_open();
    }

    if (this->snddev)
    {
        int _period;
        int i = 0;

//        snd_pcm_prepare(this->snddev);

#if 1
        err = alsa_wrapper._pcm_writei(this->snddev,
                             buf, len / framesize);
        if(err > 0)
        {
            TRACE("Wrote %d sound frames from %d given\n",
                  err, len / framesize);
        }
        else
        {
            TRACE("Unable to write data: %s\n", snd_strerror(err));
        }

#elif 0
        do
        {
            _period = (len - i) > (period * framesize)
                    ? period : (len - i) / framesize;

            err = alsa_wrapper._pcm_writei(this->snddev,
                                 &((char *)buf)[i], _period * framesize);
            if(err > 0)
            {
                //                TRACE("Wrote %d sound frames from %d given\n",
                //                      err, len / framesize);
            }
            else
            {
                TRACE("Unable to write data: %s\n", snd_strerror(err));
            }

            i+= (period * framesize);
        } while (i < len);
#else
        avail = alsa_wrapper._pcm_avail_update(snddev);
        while (avail >= period && _buf < _end) {
            if ((_end - _buf) < period*framesize)
                period = (_end - _buf) / framesize;
            alsa_wrapper._pcm_writei(snddev, _buf, period * framesize);
            _buf += avail * framesize;
            avail = snd_pcm_avail_update(snddev);
        }
#endif
    }

    #ifdef DEBUG_DATA
    char sbuf[32];
    sprintf(sbuf,"%s.pcm",this->__getenv("resource"));
    int fd = ::open(sbuf, O_APPEND | O_RDWR);
    write(fd, buf, len);
    close(fd);
    #endif

#endif
    EXIT;
    return len;
}

int
GobeeSpeaker::on_append(gobee::__x_object *parent)
{
    char _buf[32];
    ENTER;
    soundbuf = new unsigned char[1024];
    framesize = 2;
    state = 1;

    TRACE("Create file\n");

#ifdef DEBUG_DATA
    sprintf(_buf,"%s.pcm",this->__getenv("resource"));
    int fdin = ::open(_buf, O_RDWR | O_CREAT, S_IRWXU);
    close(fdin);
#endif

    EXIT;
    return 0;
}

void
GobeeSpeaker::on_release()
{
    ENTER;
    delete[] soundbuf;
    framesize = 0;
    period = 0; //
    EXIT;
}

//void
//GobeeSpeaker::on_remove()
//{
//    ENTER;
//    EXIT;
//}

//int
//GobeeSpeaker::equals(x_obj_attr_t *a)
//{
//    return (int) true;
//}

//void
//GobeeSpeaker::on_create()
//{
//}

//void
//GobeeSpeaker::finalize()
//{

//}

//void
//GobeeSpeaker::on_tree_destroy()
//{
//    ENTER;
//    EXIT;
//}

gobee::__x_object *
GobeeSpeaker::operator+=(x_obj_attr_t *attrs)
{
    register x_obj_attr_t *attr;
    bool update = false;

    ENTER;

#if 0
    for (attr = attrs->next; attr; attr = attr->next)
    {
        if (EQ("bitrate", attr->key))
        {
            bitrate = atoi(attr->val);
            TRACE("bitrate=%d\n",bitrate);
            update = true;
        }
        else if (EQ("samplesize", attr->key))
        {
            samplesize = atoi(attr->val);
            TRACE("samplesize=%d\n",samplesize);
            update = true;
        }
        else  if (EQ("framesize", attr->key))
        {
            framesize = atoi(attr->val);
            TRACE("framesize=%d\n",framesize);
            update = true;
        }

        __setattr(attr->key, attr->val);
    }
#endif
    if (update)
    {
        // this->framebuf update
    }

    EXIT;
    return this;
}


static void
___on_x_path_event(gobee::__x_object *obj)
{
    x_object_add_path(X_OBJECT(obj), X_OUT_DEVDIR "/" AUDIO_VENDOR_ID, NULL);
}

static gobee::__path_listener _spk_bus_listener;

GobeeSpeaker::GobeeSpeaker()
{
    load_alsa_lib();

    gobee::__x_class_register_xx(this, sizeof(GobeeSpeaker),
                                 "gobee:media", "audio_player");

    _spk_bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("/", (x_path_listener_t *) (void *) &_spk_bus_listener);

    TRACE("Ok!!!\n");
    return;
}

static GobeeSpeaker oproto;

#if 0
#if !defined (__clang__)
__x_plugin_visibility
__plugin_init
#else
static
#endif
void
vspk_plugin_init(void)
{
    gobee::__x_class_register_xx(&oproto, sizeof(GobeeSpeaker),
                                 "gobee:media", "audio_player");

//    _spk_bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
//    x_path_listener_add("/", (x_path_listener_t *) (void *) &_spk_bus_listener);

    TRACE("Ok!!!\n");
    return;
}

#if defined (__clang__)
class static_initializer {
    void (*_fdtor)(void);

public:
    static_initializer(void (*fctor)(void), void (*fdtor)(void)): _fdtor(fdtor){
        if(fctor) fctor();
    }
    ~static_initializer() {}
};

static static_initializer __minit(&vspk_plugin_init,NULL);
#endif
#endif
