
#include <xwlib/x_config.h>

#define DEBUG_PRFX "[ V-SOUND ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <stdio.h>

#define X_OUT_DEVDIR "__proc/out"
#define AUDIO_VENDOR_ID "QtSound_ALSA"

#include "vspeaker.h"

#define ALSA_BRATE 16000

#ifdef LINUX
int
GobeeSpeaker::alsa_open()
{
    int rc;
    unsigned int val;
    int dir;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;

    ENTER;

    /* Open PCM device for playback. */
    rc = snd_pcm_open(&this->snddev, "default",
                      SND_PCM_STREAM_PLAYBACK,  SND_PCM_NONBLOCK | SND_PCM_ASYNC);
    if (rc < 0)
    {
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
        exit(1);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);


    /* Fill it in with default values. */
    snd_pcm_hw_params_any(this->snddev, params);

    /* Set the desired hardware parameters. */
    /* Interleaved mode */
    snd_pcm_hw_params_set_access(this->snddev, params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(this->snddev, params, SND_PCM_FORMAT_S16_LE);

    val = ALSA_BRATE;
    //  snd_pcm_hw_params_set_rate_near(mctl->snddev, params, &val, &dir);
    snd_pcm_hw_params_set_rate_near(this->snddev, params, &val, &dir);

    snd_pcm_hw_params_set_channels(this->snddev, params, 1);


    /* Set period size to 32 frames. */
    frames = 0;
    snd_pcm_hw_params_set_period_size_near(this->snddev, params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(this->snddev, params);
    if (rc < 0)
    {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        exit(1);
    }

    snd_pcm_prepare(this->snddev);

    EXIT;
}
#endif

int
GobeeSpeaker::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    int err;

    ENTER;
#ifdef LINUX
    if (!this->snddev)
    {
       alsa_open();
    }

    if (this->snddev)
    {
        err = snd_pcm_writei(this->snddev, buf, len / framesize);
        if(err > 0)
        {
            TRACE("Wrote %d sound frames from %d given\n",
              err, len / framesize);
            #ifdef DEBUG_DATA
                    int fd = ::open("xspeex-remote2.pcm", O_APPEND | O_RDWR);
                    write(fd, buf, len);
                    close(fd);
            #endif
        }
        else
        {
            perror("Error writing to pipe\n");
//            TRACE("Error writing to pipe\n");
        }
    }
#endif
    EXIT;
    return len;
}

int
GobeeSpeaker::on_append(gobee::__x_object *parent)
{
    ENTER;
    soundbuf = new unsigned char[512];
    framesize = 2;
//    alsa_open();
    EXIT;
    return 0;
}

gobee::__x_object *
GobeeSpeaker::operator+=(x_obj_attr_t *attrs)
{
    register x_obj_attr_t *attr;
    bool update = false;

    ENTER;


    for (attr = attrs->next; attr; attr = attr->next)
    {
        if (EQ("bitrate", attr->key))
        {
            bitrate = atoi(attr->val);
            ::printf("GobeeSpeaker::assign(): bitrate=%d\n",bitrate);
            update = true;
        }
        else if (EQ("samplesize", attr->key))
        {
            samplesize = atoi(attr->val);
            ::printf("GobeeSpeaker::assign(): samplesize=%d\n",samplesize);
            update = true;
        }
        else  if (EQ("framesize", attr->key))
        {
            framesize = atoi(attr->val);
            ::printf("GobeeSpeaker::assign(): framesize=%d\n",framesize);
            update = true;
        }

        __setattr(attr->key, attr->val);
    }

    if (update)
    {
        // this->framebuf update
    }

    EXIT;
    return static_cast<gobee::__x_object *>(this);
}


static void
___on_x_path_event(gobee::__x_object *obj)
{
    x_object_add_path(X_OBJECT(obj), X_OUT_DEVDIR "/" AUDIO_VENDOR_ID, NULL);
}

static gobee::__path_listener _spk_bus_listener;

GobeeSpeaker::GobeeSpeaker()
{
    gobee::__x_class_register_xx(this, sizeof(GobeeSpeaker),
                                 "gobee:media", "__microphone");

    _spk_bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("/", (x_path_listener_t *) (void *) &_spk_bus_listener);

    return;
}

static GobeeSpeaker oproto;
