#ifdef WIN32
#include <Windows.h>
#endif

#include "AudioPlay.h"
#include "Logs.h"
#include "GlobalFunc.h"



#ifdef ANDROID_NDK

#include <assert.h>
#include <jni.h>
#include <string.h>

// for __android_log_print(ANDROID_LOG_INFO, "YourApp", "formatted message");
// #include <android/log.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>


// engine interfaces
static SLObjectItf engineObject = NULL;
static SLEngineItf engineEngine;

// output mix interfaces
static SLObjectItf outputMixObject = NULL;
static SLEnvironmentalReverbItf outputMixEnvironmentalReverb = NULL;

// buffer queue player interfaces
static SLObjectItf bqPlayerObject = NULL;
static SLPlayItf bqPlayerPlay;
static SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
static SLEffectSendItf bqPlayerEffectSend;
static SLMuteSoloItf bqPlayerMuteSolo;
static SLVolumeItf bqPlayerVolume;

// aux effect on the output mix, used by the buffer queue player
static const SLEnvironmentalReverbSettings reverbSettings =
SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;




#ifdef __cplusplus
extern "C"
{
#endif




// this callback handler is called every time a buffer finishes playing   
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *soundMix)  
{  
	if(bq != bqPlayerBufferQueue)
	{
		JPLAYER_LOG_ERROR("bqPlayerBufferQueue bq not eq\n");
		
		return;
	}

	if (bqPlayerBufferQueue == NULL)
	{
		JPLAYER_LOG_ERROR("bqPlayerBufferQueue null\n");
		return;
	}

	getAudioManager().doStop();
	
}  



// shut down the native audio system
void SlesShutdown()
{
	// destroy buffer queue audio player object, and invalidate all associated interfaces
	if (bqPlayerObject != NULL) {
		(*bqPlayerObject)->Destroy(bqPlayerObject);
		bqPlayerObject = NULL;
		bqPlayerPlay = NULL;
		bqPlayerBufferQueue = NULL;
		bqPlayerEffectSend = NULL;
		bqPlayerMuteSolo = NULL;
		bqPlayerVolume = NULL;
	}

	

	// destroy output mix object, and invalidate all associated interfaces
	if (outputMixObject != NULL) {
		(*outputMixObject)->Destroy(outputMixObject);
		outputMixObject = NULL;
		outputMixEnvironmentalReverb = NULL;
	}

	// destroy engine object, and invalidate all associated interfaces
	if (engineObject != NULL) {
		(*engineObject)->Destroy(engineObject);
		engineObject = NULL;
		engineEngine = NULL;
	}
}


// create the engine and output mix objects
int SlesCreateEngine( )
{
	SLresult result;

	// create engine
	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("slCreateEngine fail\n");
		SlesShutdown();
		return -1;
	}

	// realize the engine
	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("engineObject Realize fail\n");
		SlesShutdown();
		return -1;
	}

	// get the engine interface, which is needed in order to create other objects
	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("engineObject  GetInterface SL_IID_ENGINE fail\n");
		SlesShutdown();
		return -1;
	}

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[1] = {SL_BOOLEAN_FALSE};
	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("CreateOutputMix fail\n");
		SlesShutdown();
		return -1;
	}

	// realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("outputMixObject Realize fail\n");
		SlesShutdown();
		return -1;
	}

	

	// get the environmental reverb interface
	// this could fail if the environmental reverb effect is not available,
	// either because the feature is not present, excessive CPU load, or
	// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	/*result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
		&outputMixEnvironmentalReverb);
	if (SL_RESULT_SUCCESS == result) 
	{
		result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
			outputMixEnvironmentalReverb, &reverbSettings);
	}*/
	// ignore unsuccessful result codes for environmental reverb, as it is optional for this example

	return 0;
}

// create buffer queue audio player
int SlesCreateBQPlayer( 
						int rate, 
						int nChannel, 
						int bitsPerSample )
{
    SLresult result;
    // configure audio source
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq
		 = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 5};
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType = SL_DATAFORMAT_PCM;
    format_pcm.numChannels = nChannel;
    format_pcm.samplesPerSec = rate * 1000; //SL_SAMPLINGRATE_22_05;
    format_pcm.bitsPerSample = bitsPerSample;
    format_pcm.containerSize = bitsPerSample;
    if( nChannel == 2 )
	{
        format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	}
    else
	{
        format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
	}
    format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	 SLDataSource audioSrc = {&loc_bufq, &format_pcm};

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
	SLDataSink audioSnk = {&loc_outmix, NULL};

	// create audio player
	const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
		/*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
	const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
		/*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
		3, ids, req);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("CreateAudioPlayer fail\n");
		SlesShutdown();
		return -1;
	}
	// realize the player
	result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerObject Realize fail\n");
		SlesShutdown();
		return -1;
	}

	// get the play interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerObject GetInterface SL_IID_PLAY fail\n");
		SlesShutdown();
		return -1;
	}

	// get the buffer queue interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
		&bqPlayerBufferQueue);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerObject GetInterface SL_IID_BUFFERQUEUE fail\n");
		SlesShutdown();
		return -1;
	}

	// register callback on the buffer queue
	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, 
		bqPlayerCallback, NULL);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerBufferQueue RegisterCallback fail\n");
		SlesShutdown();
		return -1;
	}
	// get the effect send interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
		&bqPlayerEffectSend);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerObject GetInterface SL_IID_EFFECTSEND fail\n");
		SlesShutdown();
		return -1;
	}

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
	// get the mute/solo interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
	assert(SL_RESULT_SUCCESS == result);
#endif

	/*// get the volume interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerObject GetInterface SL_IID_VOLUME fail\n");
		SlesShutdown();
		return -1;
	}

	result = (*bqPlayerVolume)->SetMute( bqPlayerVolume, SL_BOOLEAN_FALSE );
	//    assert(SL_RESULT_SUCCESS == result);

	SLmillibel level;
	result = (*bqPlayerVolume)->GetVolumeLevel( bqPlayerVolume, &level );
	JPLAYER_LOG_ERROR("GetVolumeLevel,ret=%d,  SLmillibel:%f\n",result,level);
*/

	/*// set the player's state to playing
	result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	if (SL_RESULT_SUCCESS != result) 
	{
		JPLAYER_LOG_ERROR("bqPlayerPlay SL_PLAYSTATE_PLAYING  fail\n");
		SlesShutdown();
		return -1;
	}*/

	return 0;
}









int BwAudioCreate(
					 int rate, 
					 int nChannel, 
					 int bitsPerSample)
{
    int ret = SlesCreateEngine();
	if (ret == 0)
	{
		ret = SlesCreateBQPlayer(rate, nChannel, bitsPerSample);
	}
    
    return ret;
}

int BwAudioStart()
{
	if (bqPlayerPlay != NULL)
	{
		 SLresult result = 
			 (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
		 if (SL_RESULT_SUCCESS == result) 
		 {
			return 0;
		 }

		 JPLAYER_LOG_ERROR("SetPlayState fail:%d\n",result);
	}
	return -1;
    
}

void BwAudioStop()
{
	if (bqPlayerPlay != NULL)
	{
		SLresult result = 
	   (*bqPlayerPlay)->SetPlayState(bqPlayerPlay,  SL_PLAYSTATE_STOPPED); //SL_PLAYSTATE_PAUSED
		if (SL_RESULT_SUCCESS == result) 
		{
			return ;
		}

		JPLAYER_LOG_ERROR("SetPlayState  SL_PLAYSTATE_PAUSED fail:%d\n",result);
	}
}

int BwAudioWrite( const void* buffer, int size)
{
	if (bqPlayerBufferQueue != NULL)
	{
		SLresult result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, 
			(short *)buffer, size );
		if(SL_RESULT_SUCCESS == result)
		{
			return 0;
		}
	}

	/*if (bqPlayerBufferQueue != NULL)
	{
		static unsigned count = 0;
		if (count % 2 == 0)
		{
			SLresult result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, 
				(short *)hello, 
				sizeof(hello) );

			if(SL_RESULT_SUCCESS != result)
			{
				JPLAYER_LOG_ERROR("bqPlayerBufferQueue Enqueue fail:%d\n",result);
			}
			else
			{
				JPLAYER_LOG_ERROR("bqPlayerBufferQueue Enqueue :%d\n",result);
			}
		}
		else
		{
			SLresult result = (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, 
				(short *)android, 
				sizeof(android) );

			if(SL_RESULT_SUCCESS != result)
			{
				JPLAYER_LOG_ERROR("bqPlayerBufferQueue Enqueue fail:%d\n",result);
			}
			else
			{
				JPLAYER_LOG_ERROR("bqPlayerBufferQueue Enqueue :%d\n",result);
			}
		}

		count++;
		
	}*/
   
    return -1;
}

void BwAudioDestroy()
{
    SlesShutdown( );
}

int BwAudioGetVolume( float* left)
{
    /*SLmillibel level = 0;
	if (  left != NULL  )
	{
		SLresult result = (*bqPlayerVolume)->GetVolumeLevel( bqPlayerVolume, &level );
		if(SL_RESULT_SUCCESS == result)
		{
			*left = level;
			return 0;
		}

		JPLAYER_LOG_ERROR("GetVolumeLevel fail:%d\n",result);
		
	}*/
	return -1;
}

int BwAudioSetVolume( float left)
{
	/*if ( bqPlayerVolume != NULL)
	{
         SLresult result = (*bqPlayerVolume)->SetVolumeLevel( bqPlayerVolume, left );
		 if(SL_RESULT_SUCCESS == result)
		 {
			 return 0;
		 }

		 JPLAYER_LOG_ERROR("SetVolumeLevel fail:%d\n",result);
	}*/
	return -1;
}


#ifdef __cplusplus
}
#endif


#else

#ifdef WIN32


#ifdef __cplusplus
extern "C"
{
#endif

int BwAudioCreate(
							   int rate, 
							   int nChannel, 
							   int bitsPerSample)
{
	return 0;
}

void BwAudioDestroy()
{

}


int  BwAudioStart()
{
	return 0;
}

void BwAudioStop()
{
	return ;
}


int BwAudioWrite( const void* buffer, int size)
{
	return 0;
}


int BwAudioGetVolume( float* left)
{
	return 0;
}

int BwAudioSetVolume( float left)
{
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif

#endif


