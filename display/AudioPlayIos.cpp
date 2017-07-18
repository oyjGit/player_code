#include "AudioPlay.h"
#include "Logs.h"
#include "GlobalFunc.h"
#include "TCriticalSection.h"

#ifdef OS_MAC

#include <OpenAL/al.h>
#include <OpenAL/alc.h>



class AudioProcess 
{
public:
	AudioProcess()
		:m_mContext(NULL),
		 m_mDevice(NULL),
		 m_outSourceID(0)
	{
	}
	~AudioProcess()
	{
	}

	void setPara( int rate, 
		int nChannel, 
		int bitsPerSample)
	{
		m_rate = rate;
		m_nChannel = nChannel;
		m_bitsPerSample = bitsPerSample;
	}

	bool initOpenAL();
	void cleanUpOpenAL();

	bool openAudioFromQueue(unsigned char*data,int dataSize);

	bool playSound();
	bool pauseSound();
	bool stopSound(); 

private:
	bool updataQueueBuffer();

private:
	ALCcontext *m_mContext;
	ALCdevice  *m_mDevice;
	ALuint     m_outSourceID;
	
	int m_rate; 
	int m_nChannel;
	int m_bitsPerSample;
	comn::CriticalSection m_csPlaySound;
};

bool AudioProcess::initOpenAL()
{
	comn::AutoCritSec lock(m_csPlaySound);

	if (m_mDevice != NULL)
	{
		return true;
	}
	
	m_mDevice = alcOpenDevice(NULL);
	if (m_mDevice != NULL) 
	{
		m_mContext = alcCreateContext(m_mDevice, NULL);
		alcMakeContextCurrent(m_mContext);
	}

	alGenSources(1, &m_outSourceID);
	alSpeedOfSound(1.0);
	alDopplerVelocity(1.0);
	alDopplerFactor(1.0);
	alSourcef(m_outSourceID, AL_PITCH, 1.0f);
    ///音量
	alSourcef(m_outSourceID, AL_GAIN, 1.0f);
    ///非循环播放
	alSourcei(m_outSourceID, AL_LOOPING, AL_FALSE);
	alSourcef(m_outSourceID, AL_SOURCE_TYPE, AL_STREAMING);

	return m_mDevice != NULL;
}


void AudioProcess::cleanUpOpenAL()
{
	comn::AutoCritSec lock(m_csPlaySound);

	if (m_mDevice == NULL)
	{
		return;
	}

	
	alDeleteSources(1, &m_outSourceID);

	if (m_mContext != NULL)
	{
		alcDestroyContext(m_mContext);  
		m_mContext = NULL;
	}

	if (m_mDevice != NULL)
	{
		alcCloseDevice(m_mDevice);
		m_mDevice = NULL;
	}
	
}

bool AudioProcess::updataQueueBuffer()
{
	ALint stateVaue = 0;
	alGetSourcei(m_outSourceID, AL_SOURCE_STATE, &stateVaue);

	if (   stateVaue == AL_STOPPED 
		|| stateVaue == AL_PAUSED 
		|| stateVaue == AL_INITIAL ) 
	{
		if (m_mDevice != NULL)
		{
			if ( stateVaue != AL_PLAYING)
			{
				alSourcePlay(m_outSourceID);
			}
		}

		return false;
	}

	int processed = 0, queued = 0;
	alGetSourcei(m_outSourceID, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei(m_outSourceID, AL_BUFFERS_QUEUED, &queued);

	while(processed--)
	{
		ALuint     buffid = 0;
		alSourceUnqueueBuffers(m_outSourceID, 1, &buffid);
		alDeleteBuffers(1, &buffid);
	}

	return true;
}

bool AudioProcess::openAudioFromQueue(unsigned char*data,int dataSize)
{
	comn::AutoCritSec lock(m_csPlaySound);

	updataQueueBuffer();
    
    ALenum  error = AL_NO_ERROR;
    
    if((error = alGetError()) != AL_NO_ERROR)
	{
		//NSLog(@"error: %x\n", error);
		return false;
	}

	
	ALuint bufferID = 0;
	alGenBuffers(1, &bufferID);
    
    if((error = alGetError()) != AL_NO_ERROR)
	{
		//NSLog(@"alGenBuffers error: %x\n", error);
		return false;
	}
		
	alBufferData(bufferID, 
		AL_FORMAT_MONO16, 
		(short*)data, 
		(ALsizei)dataSize,
		m_rate);

	if((error = alGetError()) != AL_NO_ERROR) 
	{ 
		//NSLog(@"error alBufferData: %x\n", error);
		alDeleteBuffers(1, &bufferID); 
		return false;
	} 
	
	alSourceQueueBuffers(m_outSourceID, 1, &bufferID);

	if((error = alGetError()) != AL_NO_ERROR) 
	{ 
		//NSLog(@"error alSourceQueueBuffers: %x\n", error);
		alDeleteBuffers(1, &bufferID); 
		return false;
	} 
	
	

	return true;

}

bool AudioProcess::playSound()
{
	comn::AutoCritSec lock(m_csPlaySound);

	if (m_mDevice == NULL)
	{
		return false;
	}
    
    ALint stateVaue = 0;
	alGetSourcei(m_outSourceID, AL_SOURCE_STATE, &stateVaue);
	if ( stateVaue != AL_PLAYING)
	{
		alSourcePlay(m_outSourceID);
	}

    return true;
}


bool AudioProcess::pauseSound()
{
	comn::AutoCritSec lock(m_csPlaySound);

	if (m_mDevice == NULL)
	{
		return false;
	}

	alSourcePause(m_outSourceID);

	return true;
}

bool AudioProcess::stopSound()
{
	comn::AutoCritSec lock(m_csPlaySound);

	if (m_mDevice == NULL)
	{
		return false;
	}

	alSourceStop(m_outSourceID);

	return true;
}

AudioProcess g_AudioProcess;


#ifdef __cplusplus
extern "C"
{
#endif



int BwAudioCreate(
				   int rate, 
				   int nChannel, 
				   int bitsPerSample)
{
	g_AudioProcess.setPara(rate,nChannel,bitsPerSample);

	bool ok = g_AudioProcess.initOpenAL();

	return ok?0:-1;
}

void BwAudioDestroy()
{
	g_AudioProcess.cleanUpOpenAL();
}


int  BwAudioStart()
{
	bool ok = g_AudioProcess.playSound();
	return ok?0:-1;
}

void BwAudioStop()
{
	g_AudioProcess.stopSound();
	return ;
}


int BwAudioWrite( const void* buffer, int size)
{
	bool ok = g_AudioProcess.openAudioFromQueue((unsigned char*)buffer,size);
	return ok?0:-1;
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


