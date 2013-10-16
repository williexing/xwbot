
#include <x_config.h>

#define DEBUG_PRFX "[ V-SOUND ] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <stdio.h>

#define X_OUT_DEVDIR "__proc/out"
#define AUDIO_VENDOR_ID "QtSound_ALSA"

#include "vspeaker.h"

#define ALSA_BRATE 16000

void
GobeeSpeaker::playOut(SLAndroidSimpleBufferQueueItf bq, void *context)
{
  GobeeSpeaker *p = (GobeeSpeaker *)context;
  (*bq)->Enqueue(bq,(short *)p->soundbuf,p->framesize);
  TRACE("");
}

int
GobeeSpeaker::dev_open()
{
  SLresult res;
  SLEngineOption engineOption[2] = { (SLuint32) SL_ENGINEOPTION_THREADSAFE,
      (SLuint32) SL_BOOLEAN_TRUE };

  SLDataLocator_AndroidSimpleBufferQueue loc_bufq =
      {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 1};

  SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_8,
      SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
      SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN};

  const SLInterfaceID ids[] = { SL_IID_ENVIRONMENTALREVERB
      /* SL_IID_ANDROIDSIMPLEBUFFERQUEUE */ };
  const SLboolean req[] = {SL_BOOLEAN_TRUE};

  const SLEnvironmentalReverbSettings reverbSettings =
      SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;

  ENTER;
  res = slCreateEngine(&m_engineObj, 1, engineOption,
      0, NULL, NULL);

  if (m_engineObj)
    {
      res = (*m_engineObj)->Realize(m_engineObj, SL_BOOLEAN_FALSE);
TRACE("");
      res = (*m_engineObj)->GetInterface(m_engineObj, SL_IID_ENGINE, &m_engineItf);
      TRACE("");
      res = (*m_engineItf)->CreateOutputMix(m_engineItf, &m_outputObj, 0, NULL, NULL);
      TRACE("");
      res = (*m_outputObj)->Realize(m_outputObj, SL_BOOLEAN_FALSE);

      TRACE("");
      res = (*m_outputObj)->GetInterface(m_outputObj,
          SL_IID_ENVIRONMENTALREVERB, &outputMixEnvironmentalReverb);

      if (SL_RESULT_SUCCESS == res) {
          TRACE("");
          res = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
              outputMixEnvironmentalReverb, &reverbSettings);
      }

      const SLInterfaceID _ids[4] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
              /*SL_IID_MUTESOLO,*/ SL_IID_VOLUME, SL_IID_ANDROIDCONFIGURATION};
      const SLboolean _req[4] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
              /*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

      SLDataLocator_OutputMix loc_outmix =
          {SL_DATALOCATOR_OUTPUTMIX, m_outputObj};
      SLDataSink audioSnk = {&loc_outmix, NULL};
      SLDataSource audioSrc = {&loc_bufq, &format_pcm};

      TRACE("");
      res = (*m_engineItf)->CreateAudioPlayer(m_engineItf,
          &m_playerObj, &audioSrc,
          &audioSnk, 1, _ids, _req);

      SLAndroidConfigurationItf playerConfig;

      TRACE("");
      res = (*m_playerObj)->GetInterface(m_playerObj,
          SL_IID_ANDROIDCONFIGURATION, &playerConfig);

      if (SL_RESULT_SUCCESS == res) {
          SLint32 streamType = SL_ANDROID_STREAM_VOICE;
          TRACE("");
          res = (*playerConfig)->SetConfiguration(playerConfig,
              SL_ANDROID_KEY_STREAM_TYPE, &streamType, sizeof(SLint32));
      }
      TRACE("");
      res = (*m_playerObj)->Realize(m_playerObj, SL_BOOLEAN_FALSE);

      TRACE("");
      res = (*m_playerObj)->GetInterface(m_playerObj, SL_IID_PLAY,&(m_playItf));
      TRACE("");
      res = (*m_playerObj)->GetInterface(m_playerObj,
          SL_IID_BUFFERQUEUE, &(bqPlayerBufferQueue));

      TRACE("");
      res = (*bqPlayerBufferQueue)->RegisterCallback(
                            bqPlayerBufferQueue,
                            GobeeSpeaker::playOut, this);

      TRACE("");
      res = (*m_playerObj)->GetInterface(m_playerObj, SL_IID_EFFECTSEND,
              &bqPlayerEffectSend);

      TRACE("");
      res = (*m_playerObj)->GetInterface(m_playerObj, SL_IID_VOLUME,
          &bqPlayerVolume);

    }

  EXIT;
  return res;
}

int
GobeeSpeaker::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
  int err;
  unsigned char *_buf = (unsigned char *)buf;
  unsigned char *_end = _buf + len;

  ENTER;

  ::memcpy((void *)soundbuf,(const void*)buf, len);
  framesize = len/2;

  EXIT;

  return len;
}

int
GobeeSpeaker::on_append(gobee::__x_object *parent)
{
  char _buf[32];
  ENTER;
  state = 1;

  framesize = 320;
  soundbuf = new short[framesize];

  dev_open();

  TRACE("Starting player...\n");
  (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, soundbuf,
      framesize);
  int res = (*m_playItf)->SetPlayState(m_playItf, SL_PLAYSTATE_PLAYING);
  if (res != SL_RESULT_SUCCESS)
    {
      TRACE("ERROR!!! Starting player\n");
    }

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
