//
//  gobee_mac_vmic.c
//  GobeeMe
//
//  Created by артемка on 01.03.13.
//
//

#include <stdio.h>

#include <MacTypes.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioComponent.h>

#if !TARGET_OS_IPHONE
#include <CoreAudio/CoreAudio.h>
#endif

#undef DEBUG_PRFX
#include <x_config.h>
//#if TRACE_ALSA_SOUND_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-mic-iOS] "
//#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#define FRAME_BUFSIZ 1024
#define X_IN_DEVDIR "__proc/in"
#define MICROPHONE_VENDOR_ID "gobee_Microphone-Mac"

typedef struct x_vmic
{
  x_object xobj;
  struct ev_periodic mic_qwartz;
  unsigned char *framebuf;
  int sample_rate;
  int sample_size;
  AudioComponentDescription desc;
  AudioComponentInstance audioUnit;
} x_vmic_t;


static int
__xmic_check_writehandler(x_vmic_t *thiz)
{
  x_object *tmp = NULL;
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


OSStatus
recordingCallback(void *inRefCon,
                  AudioUnitRenderActionFlags *ioActionFlags,
                  const AudioTimeStamp *inTimeStamp,
                  UInt32 inBusNumber,
                  UInt32 inNumberFrames,
                  AudioBufferList *ioData)
{
  AudioBuffer buffer;
  AudioBufferList bufferList;
  OSStatus status;
  
  x_vmic_t *thiz = (x_vmic_t *)inRefCon;
  
  if (!thiz || !thiz->framebuf
      || (__xmic_check_writehandler(thiz) != 0))
    return 0;
  
  buffer.mDataByteSize = FRAME_BUFSIZ;
  buffer.mNumberChannels = 1;
  buffer.mData = thiz->framebuf;
  
  bufferList.mNumberBuffers = 1;
  bufferList.mBuffers[0] = buffer;
  
  status = AudioUnitRender(thiz->audioUnit,
                           ioActionFlags,
                           inTimeStamp,
                           inBusNumber,
                           inNumberFrames,
                           &bufferList);
  
  _WRITE(thiz->xobj.write_handler, thiz->framebuf, inNumberFrames * 2, NULL);
  
  return 0;
}

static int
initCoreAudioComponent(void *inRefCon, AudioComponentDescription *desc,
                       AudioComponentInstance *audioUnit)
{
  UInt32 siz;
  OSStatus status;
  AURenderCallbackStruct callbackStruct;
  AudioStreamBasicDescription audioFormat;
  
  desc->componentType = kAudioUnitType_Output;
#if !TARGET_OS_IPHONE
	desc->componentSubType = kAudioUnitSubType_HALOutput;
#else
  desc->componentSubType = kAudioUnitSubType_RemoteIO;
#endif
  desc->componentFlags = 0;
  desc->componentFlagsMask = 0;
  desc->componentManufacturer = kAudioUnitManufacturer_Apple;
  
  // Get component
  AudioComponent inputComponent = AudioComponentFindNext(NULL, desc);
  
  // Get audio units
  status = AudioComponentInstanceNew(inputComponent, audioUnit);
  
  UInt32 flag = -1;
  flag = 1;
  status = AudioUnitSetProperty(
                                *audioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Input,
                                1,
                                &flag,
                                sizeof(flag)
                                );
  
  
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &audioFormat,
                                sizeof(audioFormat));

  // Describe format
  audioFormat.mSampleRate			= 16000.00;
  audioFormat.mFormatID			= kAudioFormatLinearPCM;
  audioFormat.mFormatFlags		= kLinearPCMFormatFlagIsSignedInteger
  | kLinearPCMFormatFlagIsPacked ;
  audioFormat.mFramesPerPacket	= 1;
  audioFormat.mChannelsPerFrame	= 1;
  audioFormat.mBitsPerChannel		= 16;
  audioFormat.mBytesPerPacket		= 2;
  audioFormat.mBytesPerFrame		= 2;
  
  // Apply format
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output,
                                1,
                                &audioFormat,
                                sizeof(audioFormat));
  
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &audioFormat,
                                sizeof(audioFormat));

  // Set output callback
//  memset(&callbackStruct,0,sizeof(callbackStruct));
  callbackStruct.inputProc = recordingCallback;
  callbackStruct.inputProcRefCon = inRefCon;
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioOutputUnitProperty_SetInputCallback,
                                kAudioUnitScope_Output,
                                1,
                                &callbackStruct,
                                sizeof(callbackStruct));

  flag = 0;
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_ShouldAllocateBuffer,
                                kAudioUnitScope_Output,
                                1,
                                &flag,
                                sizeof(flag));
  
  status = AudioUnitInitialize(*audioUnit);
  
  return status;
}

#define AU_BRATE 16000
static int
open_au_dev(x_vmic_t *thiz)
{
  return initCoreAudioComponent(thiz,&thiz->desc,&thiz->audioUnit);
}

static void
release_au_dev(x_vmic_t *thiz)
{
}

static int
x_mic_on_append(x_object *so, x_object *parent)
{
  OSStatus status;
  x_vmic_t *thiz = (x_vmic_t *) so;
  
  ENTER;
  
  TRACE("\n");
  
  thiz->framebuf = x_malloc (FRAME_BUFSIZ);
  
  _ASET(so,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
  _ASET(so,_XS("Descr"),_XS("Micriphone"));
  
  open_au_dev(thiz);
  
  status = AudioOutputUnitStart(thiz->audioUnit);
  
  EXIT;
  return 0;
}

static void
x_mic_on_remove(x_object *so)
{
  OSStatus status;
  x_vmic_t *thiz = (x_vmic_t *) so;
  
  ENTER;
  
  TRACE("\n");
  
  status = AudioOutputUnitStop(thiz->audioUnit);
  
  if(thiz->framebuf)
  {
    x_free(thiz->framebuf);
    thiz->framebuf = NULL;
  }
  
  thiz->mic_qwartz.data = NULL;
  release_au_dev(thiz);
  
  EXIT;
  return;
}

static x_object *
x_mic_on_assign(x_object *thiz_, x_obj_attr_t *attrs)
{
  ENTER;
  
  x_object_default_assign_cb(thiz_, attrs);
  
  EXIT;
  return thiz_;
}

static void
x_mic_exit(x_object *so)
{
  x_vmic_t *thiz = (x_vmic_t *) so;
  ENTER;
  
  TRACE();
  
  //    release_au_dev(thiz);
  
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

#if !TARGET_OS_IPHONE
__x_plugin_visibility
__plugin_init
#endif
void
x_mic_init(void)
{
  ENTER;
  
  TRACE("x_mic_init %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Handler OK\n");
  
  xmic_class.cname = _XS("microphone");
  xmic_class.psize = (sizeof(x_vmic_t) - sizeof(x_object));
  xmic_class.on_assign = &x_mic_on_assign;
  xmic_class.on_append = &x_mic_on_append;
  xmic_class.finalize = &x_mic_exit;
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

