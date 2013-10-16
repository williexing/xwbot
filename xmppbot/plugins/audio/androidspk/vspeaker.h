#ifndef GobeeSpeaker_H
#define GobeeSpeaker_H

#include <xwlib++/x_objxx.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

class GobeeSpeaker : public gobee::__x_object
{
    int bitrate;
    int samplesize;
    int period;
    int state;
    SLObjectItf m_engineObj;
    SLEngineItf m_engineItf;
    SLObjectItf m_outputObj;
    SLObjectItf m_playerObj;
    SLPlayItf m_playItf;
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

    SLEnvironmentalReverbItf outputMixEnvironmentalReverb;

    SLEffectSendItf bqPlayerEffectSend;
//    SLMuteSoloItf bqPlayerMuteSolo;
    SLVolumeItf bqPlayerVolume;

    // Create SLObjectItf
public:
    short *soundbuf;
    int framesize;

public:
    GobeeSpeaker();
    int dev_open();
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

    static void playOut(SLAndroidSimpleBufferQueueItf bq, void *context);

};

#endif // GobeeSpeaker_H
