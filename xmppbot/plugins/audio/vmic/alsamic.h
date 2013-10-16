#ifndef GobeeSpeaker_H
#define GobeeSpeaker_H

#include <xwlib++/x_objxx.h>

#define LINUX

#ifdef LINUX
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#endif

class GobeeSpeaker : public gobee::__x_object
{
    int framesize;
    int bitrate;
    int samplesize;
    volatile unsigned char *soundbuf;

#ifdef LINUX
    snd_pcm_t *snddev;
#endif

public:
    GobeeSpeaker();
    int alsa_open();
    virtual int on_append(gobee::__x_object *parent);
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    virtual __x_object *operator+=(x_obj_attr_t *); // on_assign
};

#endif // GobeeSpeaker_H
