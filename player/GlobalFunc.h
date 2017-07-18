#ifndef GLOBAL_FUNC_H_
#define GLOBAL_FUNC_H_

#ifdef WIN32
#include <Windows.h>
#endif


#include "VideoManager.h"
#include "AudioManager.h"
#include "AvSync.h"

AvSyncManager &getAvSyncManager();

AudioManager  &getAudioManager();

VideoManager  &getVideoManager();


#endif /* GLOBAL_FUNC_H_ */



