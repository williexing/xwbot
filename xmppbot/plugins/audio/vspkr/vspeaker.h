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
    int period;
    int state;
    volatile unsigned char *soundbuf;

#ifdef LINUX
    snd_pcm_t *snddev;
#endif

public:
    GobeeSpeaker();
    int alsa_open();
    virtual int on_append(gobee::__x_object *parent);
    virtual void on_release();
//    virtual void on_remove();
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    virtual gobee::__x_object *operator+=(x_obj_attr_t *); // on_assign
//    virtual int equals(x_obj_attr_t *);
//    virtual void on_tree_destroy();
//    virtual void on_create();

//    virtual void finalize();
//    virtual int classmatch(x_obj_attr_t *);

};

#endif // GobeeSpeaker_H
