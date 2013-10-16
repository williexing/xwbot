//
//  gobee_mac_dev.c
//  GobeeMe
//
//  Created by артемка on 01.03.13.
//
//

#include <stdio.h>

#include <MacTypes.h>

#include "gobee_mac_dev.h"

#define kOutputBus 0
#define kInputBus 1

#define checkStatus(x) do { if(x<0) return -1; } while (0)

OSStatus
recordingCallback(void *inRefCon,
                  AudioUnitRenderActionFlags *ioActionFlags,
                  const AudioTimeStamp *inTimeStamp,
                  UInt32 inBusNumber,
                  UInt32 inNumberFrames,
                  AudioBufferList *ioData);


int
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
  
  // Enable IO for recording
  UInt32 flag = 1;
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Input,
                                kInputBus,
                                &flag,
                                sizeof(flag));
  checkStatus(status);
  
  // Enable IO for playback
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Output,
                                kOutputBus,
                                &flag,
                                sizeof(flag));
  checkStatus(status);
  
  // Describe format
  audioFormat.mSampleRate			= 16000.00;
  audioFormat.mFormatID			= kAudioFormatLinearPCM;
  audioFormat.mFormatFlags		= kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  audioFormat.mFramesPerPacket	= 1;
  audioFormat.mChannelsPerFrame	= 1;
  audioFormat.mBitsPerChannel		= 16;
  audioFormat.mBytesPerPacket		= 2;
  audioFormat.mBytesPerFrame		= 2;
  
  // Apply format
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output,
                                kInputBus,
                                &audioFormat,
                                sizeof(audioFormat));
  checkStatus(status);
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                kOutputBus,
                                &audioFormat,
                                sizeof(audioFormat));
  checkStatus(status);
  
  
  // Set input callback
  callbackStruct.inputProc = recordingCallback;
  callbackStruct.inputProcRefCon = inRefCon;
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioOutputUnitProperty_SetInputCallback,
                                kAudioUnitScope_Output,
                                kInputBus,
                                &callbackStruct,
                                sizeof(callbackStruct));
  checkStatus(status);
  
  // Set output callback
//  callbackStruct.inputProc = playbackCallback;
//  callbackStruct.inputProcRefCon = inRefCon;
//  status = AudioUnitSetProperty(*audioUnit,
//                                kAudioUnitProperty_SetRenderCallback,
//                                kAudioUnitScope_Global,
//                                kOutputBus,
//                                &callbackStruct,
//                                sizeof(callbackStruct));
//  checkStatus(status);
  
  // Disable buffer allocation for the recorder (optional - do this if we want to pass in our own)
  flag = 0;
  status = AudioUnitSetProperty(*audioUnit,
                                kAudioUnitProperty_ShouldAllocateBuffer,
                                kAudioUnitScope_Output,
                                kInputBus,
                                &flag,
                                sizeof(flag));
  
  // TODO: Allocate our own buffers if we want
  
  // Initialise
  status = AudioUnitInitialize(*audioUnit);
  checkStatus(status);
}

