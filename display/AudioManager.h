#ifndef AUDIO_MANAGER_H_
#define AUDIO_MANAGER_H_

#include "NetPosaJPlayerSDK.h"
#include "TMemQueue.h"
#include "TThread.h"
#include "TEvent.h"
#include "TCriticalSection.h"


#ifdef G726
#include "G726Decode.h"
#else
#include "OpusDecode.h"
#include "OpusEncode.h"
#include "AACDecode.h"
#include "AACEncode.h"
#ifdef SPEEX
#include "SpeexDecode.h"
#endif
#endif

#include <semaphore.h>

class AudioManager : public comn::CMemQueue,public comn::Thread
{
public:
    AudioManager();
    virtual ~AudioManager();

	LONG playSound();
	LONG stopSound();

	LONG setVolume(unsigned int  volume);
	LONG getVolume(unsigned int*  pVolume);

	///����ý�����
	int writeMediaPacket(const StreamPacket& packet) ;

	
	virtual bool open() ;

	///֪ͨ����ý����ݽ���
	virtual void close() ;

	 virtual int run();

	 virtual void doStop() 
	 {
		 m_eventWaiter.post();
//		 sem_post(&_sem_player);
	 }

	 bool  pause(bool hasPause);
	 bool  setScale(float scale);

	 bool clearAllBuffer(int64_t lastAudioPts = 0);

	 bool encodeAudio(StreamPacket& packet);

	 int setAudioEncodePara(int sample_rate,int channel,int bitrates,int fps);
	 int setAudioDecodePara(int sample_rate,int channel,int bitrates);

	 bool doPcm(StreamPacket& pcm);

	 void setVideoStreamMode(int p_Mode, int p_MediaBufferLength);

	 void  setCurrentTime(long p_CurrentTime);

	 void seekSuccess();

	 static int AudioManagerCtrlHandler(int opcode, void *context);

private:
	 unsigned int Sys_GetTickCount1(void);

private:
	bool m_hasInit;
	bool m_playSound;
	bool m_firstFrame;
	comn::Event m_eventWaiter;
//	sem_t _sem_player;
	volatile int _stop_speaker;
	bool m_hasPause;

#ifdef G726
	G726Decode m_g726Decoder;
#else
	AACDecode  m_AACDecode;
	AACEncode  m_AACEncode;
	OpusDecode m_opusDecoder;
	OpusEncode m_opusEncoder;
#ifdef SPEEX
	SpeexDecode m_SpeexDecode;
#endif
#endif

	int m_sampleRate;
	int m_channel;
	bool m_openAudio;
	float m_scale;

//	void *m_pNsHandle;

	unsigned long long startPts;

	int startTime;
	
	unsigned long long lastTime;

	int videoStreamMode;

	unsigned long long m_lastpushtime;

	unsigned long long m_DropStreamTime;

	bool m_SeekSuccess;

};



/////////////////////////////////////////////////////////////////////////////

#endif /* AUDIO_MANAGER_H_ */
