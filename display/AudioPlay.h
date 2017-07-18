#ifndef AUDIO_PLAY_H
#define AUDIO_PLAY_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif


int BwAudioCreate( int rate, 
					 int nChannel, 
					 int bitsPerSample);
void BwAudioDestroy();


int  BwAudioStart();
void BwAudioStop();


int BwAudioWrite( const void* buffer, int size);


int BwAudioGetVolume( float* left);
int BwAudioSetVolume( float left);



#ifdef __cplusplus
}
#endif


#endif




