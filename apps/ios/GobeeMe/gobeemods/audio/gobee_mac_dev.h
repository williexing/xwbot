//
//  gobee_mac_dev.h
//  GobeeMe
//
//  Created by артемка on 01.03.13.
//
//

#ifndef GobeeMe_gobee_mac_dev_h
#define GobeeMe_gobee_mac_dev_h

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AudioComponent.h>

typedef struct _gobee_mac_audio_dev
{
  
} gobee_mac_audio_dev_t;

int
initCoreAudioComponent(void *inRefCon, AudioComponentDescription *desc,
                       AudioComponentInstance *audioUnit);

#endif
