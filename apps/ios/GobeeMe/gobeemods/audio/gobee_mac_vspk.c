//
//  gobee_mac_vspk.c
//  GobeePhone
//
//  Created by артемка on 13.04.13.
//  Copyright (c) 2013 CrossLine Media, LLC. All rights reserved.
//

#include <stdio.h>

#include <MacTypes.h>

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioComponent.h>


#include <CoreAudio/CoreAudioTypes.h>

#undef DEBUG_PRFX
#include <x_config.h>
//#if TRACE_ALSA_SOUND_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-spk-iOS] "
//#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>


#define CIRCBUFSIZ (8192*4)
#define FRAME_BUFSIZ 1024
#define X_IN_DEVDIR "__proc/in"
#define SPK_VENDOR_ID "gobee_spkrophone-Mac"

#define AU_BRATE 16000

#define kOutputBus 0
#define kInputBus 1

#define checkStatus(x) do { if(x<0) return -1; } while (0)

typedef struct x_spk
{
  x_object xobj;
  struct ev_periodic spk_qwartz;
  unsigned char *framebuf;
  unsigned int bufsiz;
  unsigned int bufhead;
  unsigned int buftail;
  int sample_rate;
  int sample_size;
  AudioComponentDescription desc;
  AudioComponentInstance audioUnit;
  
  circular_buffer_t circbuf;
  
} x_spk_t;


static OSStatus
playOutCallback(void *inRefCon,
                AudioUnitRenderActionFlags *ioActionFlags,
                const AudioTimeStamp *inTimeStamp,
                UInt32 inBusNumber,
                UInt32 inNumberFrames,
                AudioBufferList *ioData)
{
  OSStatus status = -1;
  int bytes = 0;
  
  x_spk_t *thiz = (x_spk_t *)inRefCon;
  
  x_memset(ioData->mBuffers[0].mData, 0, ioData->mBuffers[0].mDataByteSize);
  
  if (!thiz)
    return 0;
  
  if (thiz->circbuf.start)
    bytes = circbuf_length(&thiz->circbuf);
  
  if( ioData->mNumberBuffers
     && thiz->circbuf.start
     && inNumberFrames*2 <= bytes
     )
  {
    bytes = circbuf_read(&thiz->circbuf,ioData->mBuffers[0].mData, inNumberFrames*2);
    status = 0;
  }
  
  return status;
}


static int
initCoreAudioComponent(void *inRefCon, AudioComponentDescription *desc,
                       AudioComponentInstance *audioUnit)
{
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
  checkStatus(status);
  
  UInt32 flag = 1;
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Output,
                                0,
                                &flag,
                                sizeof(flag));

  status = AudioUnitSetProperty(
                                *audioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Input,
                                1,
                                &flag,
                                sizeof(flag)
                                );

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
                                kAudioUnitScope_Input,
                                0,
                                &audioFormat,
                                sizeof(audioFormat));
  
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output,
                                1,
                                &audioFormat,
                                sizeof(audioFormat));

  // Set output callback
  x_memset(&callbackStruct,0,sizeof(callbackStruct));
  callbackStruct.inputProc = playOutCallback;
  callbackStruct.inputProcRefCon = inRefCon;
  
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Global,
                                0,
                                &callbackStruct,
                                sizeof(callbackStruct));
  
  
  status = AudioUnitInitialize(*audioUnit);
  
  return 0;
}



static int
open_au_dev(x_spk_t *thiz)
{
  return  initCoreAudioComponent(thiz,&thiz->desc,&thiz->audioUnit);
}

static void
release_au_dev(x_spk_t *thiz)
{
}

static int
x_spk_on_append(x_object *so, x_object *parent)
{
  OSStatus status;
  x_spk_t *thiz = (x_spk_t *) so;
  
  ENTER;
  
  TRACE("\n");
  
  //  thiz->framebuf = x_malloc (FRAME_BUFSIZ);
  circbuf_init(&thiz->circbuf,CIRCBUFSIZ);
  
  _ASET(so,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
  _ASET(so,_XS("Descr"),_XS("spkriphone"));
  
  open_au_dev(thiz);
  
  status = AudioOutputUnitStart(thiz->audioUnit);
  
  EXIT;
  return 0;
}

static void
x_spk_on_remove(x_object *so)
{
  OSStatus status;
  x_spk_t *thiz = (x_spk_t *) so;
  
  ENTER;
  
  TRACE("\n");
  
  status = AudioOutputUnitStop(thiz->audioUnit);
  
  thiz->spk_qwartz.data = NULL;
  release_au_dev(thiz);
  
  circbuf_deinit(&thiz->circbuf);
  
  if(thiz->framebuf)
  {
    x_free(thiz->framebuf);
    thiz->framebuf = NULL;
  }
  
  EXIT;
  return;
}

static x_object *
x_spk_on_assign(x_object *thiz_, x_obj_attr_t *attrs)
{
  ENTER;
  
  x_object_default_assign_cb(thiz_, attrs);
  
  EXIT;
  return thiz_;
}

static void
x_spk_exit(x_object *_thiz)
{
  ENTER;
  
  EXIT;
}


static int
x_spk_try_write(x_object *xobj,
                void *framedata, u_int32_t siz, x_obj_attr_t *attrs)
{
  //  time_t stamp = time(NULL);
  x_spk_t *thiz = (x_spk_t *)(void *) xobj;
  
  circbuf_write(&thiz->circbuf, framedata, siz);
  return siz;
}

static struct xobjectclass xspk_class;

static void
___spk_on_x_path_event(void *_obj)
{
  x_object *obj = _obj;
  x_object *ob;
  
  ENTER;
  
  ob = x_object_add_path(obj, X_IN_DEVDIR "/" SPK_VENDOR_ID, NULL);
  
  EXIT;
}

static struct x_path_listener spk_bus_listener;

#if !TARGET_OS_IPHONE
__x_plugin_visibility
__plugin_init
#endif
void
x_spk_init(void)
{
  ENTER;
  
  TRACE("x_spk_init %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% Handler OK\n");
  
  xspk_class.cname = _XS("audio_player");
  xspk_class.psize = (sizeof(x_spk_t) - sizeof(x_object));
  xspk_class.on_assign = &x_spk_on_assign;
  xspk_class.on_append = &x_spk_on_append;
  xspk_class.finalize = &x_spk_exit;
  xspk_class.on_remove = &x_spk_on_remove;
  xspk_class.try_write = &x_spk_try_write;
  
  x_class_register_ns(xspk_class.cname, &xspk_class, _XS("gobee:media"));
  
  spk_bus_listener.on_x_path_event = &___spk_on_x_path_event;
  
  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &spk_bus_listener);
  EXIT;
  return;
}

