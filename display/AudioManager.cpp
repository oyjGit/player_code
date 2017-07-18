#ifdef WIN32
#include <Windows.h>
#endif

#ifdef OS_MAC
#include <sys/time.h>
#endif
#include <unistd.h>

#include "Logs.h"
#include "AudioPlay.h"
#include "GlobalFunc.h"
#include "PlayerConfig.h"
//#ifndef OS_MAC
//#include "NoiseSuppression/noise_suppression.h"
//#endif

extern int PushFarendPCM(void* packet, unsigned int size);

//#define AUDIO_TEST
#ifdef AUDIO_TEST
extern void CloudPlatformCtrl(int opcode, void *context);
int AudioManager::AudioManagerCtrlHandler(int opcode, void *context)
{
	if (opcode == 0) {
		getAudioManager().push(*(StreamPacket*)context);
	} else if (opcode == 1) {
		if ((int)context == 0) {
			getAudioManager().close();
		} else {
			getAudioManager().playSound();
		}
	} else if (opcode == 2) {
		struct audiocontext {
			StreamPacket aac;
			StreamPacket pcm;
		};
		struct audiocontext* ac = (struct audiocontext*)context;
		if (ac) {
			if (!getAudioManager().m_AACDecode.writePacket(ac->aac, ac->pcm)) {
				ac->pcm.size = 0;
			}
		}
	}
	return 0;
}
#endif

//#define SPEEX 1

AudioManager::AudioManager()
  :m_hasInit(false),
   m_playSound(false),
   m_firstFrame(true),
   m_hasPause(false),
#ifdef G726
   m_sampleRate(8000),
#else
   m_sampleRate(11025),
//    m_sampleRate(AUDIO_SAMPLE_RATES),
#endif
   m_channel(AUDIO_CHANNEL),
   m_openAudio(false),
//   m_pNsHandle(NULL),
   m_scale(1.0),
   startPts(0),
   startTime(0),
   videoStreamMode(1),
   lastTime(0),
   m_lastpushtime(0),
   m_DropStreamTime(0),
   _stop_speaker(0),
   m_SeekSuccess(false)
{
//	sem_init(&_sem_player, 0, 0);
#ifdef AUDIO_TEST
	CloudPlatformCtrl(0, (void*)AudioManagerCtrlHandler);
#endif
}

AudioManager::~AudioManager()
{
	if (m_hasInit)
	{
		stop();

		BwAudioDestroy();

		unInit();

//#ifndef OS_MAC
//		if (m_pNsHandle != NULL)
//		{
//			WebRtcNs_Free((NsHandle *)m_pNsHandle);
//			m_pNsHandle = NULL;
//		}
//#endif

		m_hasInit = false;
	}
//	sem_destroy(&_sem_player);
}


bool AudioManager::open()
{
	if (!m_hasInit)
	{
		bool ok = init(AUDIO_MAX_PACKET_POOL * KB);
		if (!ok)
		{
			JPLAYER_LOG_ERROR("AudioManager init queue fail\n");
			return false;
		}


        start();

		m_hasInit = true;
	}

	JPLAYER_LOG_INFO("AudioManager init ok\n");
	clearAllBuffer();

	return true;
}

bool AudioManager::doPcm(StreamPacket& outpcm)
{
	///�������ƣ��Զ������㷨����
//#ifndef OS_MAC
//	if (m_pNsHandle != NULL)
//	{
//		uint16_t length = m_sampleRate/100*2;
//		if (outpcm.size % length != 0)
//		{
//			return true;
//		}
//
//		int timer = outpcm.size/length;
//		for (int i = 0; i < timer; i++)
//		{
//			int err = WebRtcNs_Process((NsHandle *)m_pNsHandle,
//               (short*)(outpcm.data + length * i),
//               NULL,
//               (short*)(outpcm.data + length * i),
//               NULL);
//		}
//	}
//#endif

	return true;
}

unsigned int AudioManager::Sys_GetTickCount1(void)
{
    struct timeval aika;
    gettimeofday(&aika,NULL);
    unsigned long msecs = aika.tv_sec * 1000;
    msecs += (aika.tv_usec / 1000);
    return msecs;
}

static uint64_t GetTickCount(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec*1000+ts.tv_nsec/1000000);
}

static int GetSamplerateAndChannelFromADTS(unsigned char* adts, unsigned int size, unsigned int* samplerate, unsigned int* channel) {
	int samplerates[] = {
			96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350
	};
	unsigned int sample_rate_idx;
	unsigned int chn;

	if (size < 7) {
		return -1;
	}

	sample_rate_idx = (unsigned int)(adts[2]&0x3c)>>2;
	chn = ((unsigned int)(adts[2]&0x1)<<2)|((unsigned int)(adts[3]&0xc0)>>6);

	if (sample_rate_idx >= sizeof(samplerates)/sizeof(samplerates[0])) {
		return -1;
	}

	if (samplerate)
		*samplerate = samplerates[sample_rate_idx];
	if (channel)
		*channel = chn;
	return 0;
}

#define AUDIO_MAX_DELAY_TIME 2000 //ms
#if 0
int AudioManager::run()
{
	StreamPacket packet;
	StreamPacket pcmpacket;
	uint64_t lastsystime = 0;
	uint64_t lasttimestamp = 0;
	uint64_t currsystime;
	uint64_t difftime;

	while (!m_canExit)
	{
		if (_stop_speaker) {
			BwAudioStop();
			BwAudioDestroy();
			_stop_speaker = 0;
			m_playSound = false;
			m_openAudio = false;
			lastsystime = 0;
			lasttimestamp = 0;
			JPLAYER_LOG_INFO("====BwAudioDestroy2====");
		}

		if (!popTimeWait(packet, 1000)) {
			continue;
		}
//		JPLAYER_LOG_INFO("Pop one audio %llu", packet.timestamp);

		if (!m_playSound && !m_firstFrame)
		{
			release(packet);
			continue;
		}

		// drop frame
		if (lastsystime) {
			if (packet.timestamp < lasttimestamp) {
				lastsystime = 0;
				lasttimestamp = 0;
			} else {
				currsystime = GetTickCount();
				uint64_t droptime = packet.timestamp + AUDIO_MAX_DELAY_TIME;
				if ((currsystime-lastsystime) > (droptime-lasttimestamp)) {
					JPLAYER_LOG_INFO("drop audio frames before timestamp %llu, first frame timestamp %llu", droptime, lasttimestamp);
					do {
						release(packet);
						if (!popTimeWait(packet, 10)) {
							break;
						}
					} while (packet.timestamp < droptime);
					lastsystime = 0;
					lasttimestamp = 0;
					continue;
				}
			}
		}

		if (!m_openAudio) {
			unsigned int samplerate, channel;
			if (GetSamplerateAndChannelFromADTS(packet.data, packet.size, &samplerate, &channel)) {
				JPLAYER_LOG_INFO("GetSamplerateAndChannelFromADTS failed");
				return JPLAYER_MSG_NO_INIT_FAIL;
			}
			m_sampleRate = samplerate;
			JPLAYER_LOG_INFO("GetSamplerateAndChannelFromADTS: samplerate=%u, channel=%u", samplerate, channel);
			if (BwAudioCreate(samplerate, channel, 16) != 0)
			{
				JPLAYER_LOG_INFO("AudioManager BwAudioCreateSL  fail\n");
				return JPLAYER_MSG_NO_INIT_FAIL;
			}
			m_openAudio = true;
			m_firstFrame = false;
		}
		if (!m_playSound) {
			if (0 == BwAudioStart()) {
				JPLAYER_LOG_INFO("====BwAudioStart2====");
				m_playSound = true;
			}
		}

		if (!m_AACDecode.writePacket(packet, pcmpacket)) {
			release(packet);
			continue;
		}
		release(packet);

		currsystime = GetTickCount();
		if (lastsystime == 0) {
			lastsystime = currsystime;
			lasttimestamp = pcmpacket.timestamp;
		} else {
			difftime = currsystime - lastsystime;
			if (difftime < pcmpacket.timestamp-lasttimestamp) {
				difftime = pcmpacket.timestamp - lasttimestamp - difftime;
//				JPLAYER_LOG_INFO("audio diff time: %llu  %llu", difftime, pcmpacket.timestamp);
				usleep(difftime*1000);
			}
		}

		BwAudioWrite((char *)pcmpacket.data, pcmpacket.size);

		PushFarendPCM((void*)pcmpacket.data, pcmpacket.size);

//		JPLAYER_LOG_INFO("audio play diff time %d", (int)((int64_t)GetTickCount()-0/*_difftime*/-(int64_t)pcmpacket.timestamp));

		sem_wait(&_sem_player);

	}
	return 0;
}
#else
int AudioManager::run()
{
	int timeWaiter = AUDIO_DELAY_TIME;

	while (!m_canExit)
	{
//		if(videoStreamMode == 1 || videoStreamMode == 3)
//			timeWaiter = 10;
//		else
//			timeWaiter = AUDIO_DELAY_TIME;


		if (_stop_speaker) {
			BwAudioStop();
			BwAudioDestroy();
			_stop_speaker = 0;
			m_playSound = false;
			m_openAudio = false;
//			lastsystime = 0;
//			lasttimestamp = 0;
			JPLAYER_LOG_INFO("====BwAudioDestroy2====");
			int64_t lastVideoPts;
			getAvSyncManager().controlAudioPts(0,1,lastVideoPts);
		}

		///����������Ƶ������
		if (m_playSound&&!m_firstFrame)
		{
			///�ȴ���һ֡��ƵPCM��������źţ�IOS��OPENALʹ��æ��ѯ����
#ifndef OS_MAC
			m_eventWaiter.timedwait(timeWaiter);
#endif
		}


		///�û�������ָͣ��
		if (m_hasPause)
		{
			m_eventWaiter.timedwait(10);
			continue;
		}

#ifdef OS_MAC
	    comn::Time tsElapse1;
#endif

		///ȡ��Ƶ���
		StreamPacket packet;
		if (!popTimeWait(packet,timeWaiter))
		{
#ifdef AUDIO_TEST
			if (m_openAudio) {
				CloudPlatformCtrl(1, (void*)0);
			}
#endif
			continue;
		}

		///û����������������ͷ����
		if (!m_playSound&&!m_firstFrame)
		{
			release(packet);
			continue;
		}


		if (!m_openAudio)
		{
			unsigned int samplerate, channel;
			int ret = 0;
			if (GetSamplerateAndChannelFromADTS(packet.data, packet.size, &samplerate, &channel)) {
				JPLAYER_LOG_INFO("GetSamplerateAndChannelFromADTS failed");
				return JPLAYER_MSG_NO_INIT_FAIL;
			}
			m_sampleRate = samplerate;
			JPLAYER_LOG_INFO("GetSamplerateAndChannelFromADTS: samplerate=%u, channel=%u", samplerate, channel);

			ret = BwAudioCreate(samplerate,1,16);
			if (ret != 0)
			{
				JPLAYER_LOG_INFO("AudioManager BwAudioCreateSL  fail\n");
				return JPLAYER_MSG_NO_INIT_FAIL;
			}

//#ifndef OS_MAC
//
//			WebRtcNs_Create((NsHandle **)&m_pNsHandle);
//			if (m_pNsHandle != NULL)
//			{
//				WebRtcNs_Init((NsHandle *)m_pNsHandle, m_sampleRate);
//				WebRtcNs_set_policy((NsHandle *)m_pNsHandle, 1);
//			}
//
//#endif
			m_firstFrame = false;
			m_openAudio = true;
		}

		if (!m_playSound)
		{
			int ret = BwAudioStart();

			m_playSound = (ret == 0);
		}


#ifndef AUDIO_TEST

//		if(videoStreamMode == 1 || videoStreamMode == 3)
		{
			int timedelay = 0;
			if(videoStreamMode == 1 || videoStreamMode == 3)
			{
				if(packet.timestamp < lastTime && lastTime>0xff0000)
				{
					JPLAYER_LOG_INFO("time Rollback audio");
					packet.timestamp += 0xfffffe;
				}
			}

			if((packet.timestamp>lastTime+5000 || packet.timestamp<lastTime-5000) && m_SeekSuccess) //录像seek
			{
				JPLAYER_LOG_INFO("audio time seek success on record :%llu",packet.timestamp);
				m_SeekSuccess = false;
				startPts = packet.timestamp;
				startTime = Sys_GetTickCount1();
			}
			else if(packet.timestamp>lastTime+5000 && !m_SeekSuccess && videoStreamMode == 2)
			{
				JPLAYER_LOG_INFO("audio time skip on record :%llu",packet.timestamp);
				startPts = packet.timestamp;
				startTime = Sys_GetTickCount1();
			}


			int currentDropTime = getAvSyncManager().getAudioDefaultDropTime();
			if(currentDropTime > 0 && m_DropStreamTime == 0)
			{
				JPLAYER_LOG_INFO("Drop audio frame time :%d",currentDropTime);
				m_DropStreamTime = packet.timestamp;
				int droptime = 0;
				unsigned int packettime = packet.timestamp;
				while(packettime - m_DropStreamTime < currentDropTime)
				{
					droptime += (packettime -lastTime);
					lastTime = packettime;
					release(packet);
					if (!popTimeWait(packet,timeWaiter))
					{
						continue;
					}
					packettime = packet.timestamp;
				}

				if(getAvSyncManager().getDropType()==1)
				{
					JPLAYER_LOG_INFO("start audio time drop :%d \n",droptime);
					startTime-=droptime;
				}

				m_DropStreamTime = 0;
				getAvSyncManager().setAudioDefaultDropTime();
			}

			timedelay = getAvSyncManager().getCurrentDelayTime();
//			JPLAYER_LOG_INFO("startPts  audio:%llu  ",packet.timestamp);
			if (startPts == 0) startPts = packet.timestamp;
			if (startTime == 0) startTime = Sys_GetTickCount1();

			int nCount = 0;
			while((Sys_GetTickCount1() - startTime < (packet.timestamp-startPts)+timedelay)&&(_stop_speaker==0))
			{
//				usleep(2000);
				m_eventWaiter.timedwait(2);
				nCount ++;
				if(nCount >= 500*60)
				{
					JPLAYER_LOG_INFO("timestamp interval is too big than 60s :%d startPts:%llu framePts:%llu",timedelay,startPts,packet.timestamp);
					break;
				}
			}
		}
#endif
		///������Ƶ
		StreamPacket pcmpacket;
//#ifdef G726
//		bool ok = m_g726Decoder.writePacket(packet,pcmpacket);
//#else

#ifdef SPEEX
		bool ok = m_SpeexDecode.writePacket(packet,pcmpacket);
#else

		bool ok = m_AACDecode.writePacket(packet,pcmpacket);

//		bool ok = m_opusDecoder.writePacket(packet,pcmpacket);
#endif

		///����ɹ�
		if ( ok )
		{
			///����pcm
//			doPcm(pcmpacket);

			///�͸���豸ȥ����
#ifdef AUDIO_TEST
			JPLAYER_LOG_INFO("BwAudioWrite size=%d, timestamp=%llu, data=[%02hhx %02hhx %02hhx %02hhx]",
					pcmpacket.size, pcmpacket.timestamp, pcmpacket.data[0], pcmpacket.data[1], pcmpacket.data[2], pcmpacket.data[3]);
#endif
			int ret = BwAudioWrite((char *)(pcmpacket.data),pcmpacket.size);
			if (ret != 0)
			{
#ifdef OS_MAC
				///IOS������ȥһ�β���
				m_eventWaiter.timedwait(40);
				ret = BwAudioWrite((char *)(pcmpacket.data),pcmpacket.size);
#endif
			}

			PushFarendPCM((void*)pcmpacket.data, pcmpacket.size);

//			JPLAYER_LOG_INFO("get live timestamp audio :%llu",pcmpacket.timestamp);
		}

//		if(videoStreamMode == 2)
//		{
//#ifdef OS_MAC
//			int timeinterval = 0;
//			if(lastTime > 0 )
//				timeinterval = (int)(packet.timestamp - lastTime);
//
//			if (timeinterval < 1 || timeinterval > 5000)
//			{
//				timeinterval = 70;
//			}
//
//			int tsElapseTimer = tsElapse1.getElapse();
//
//			if(timeinterval>tsElapseTimer)
//			{
//				//						JPLAYER_LOG_INFO("timewait :%d  :%d   %d",timeinterval,tsElapseTimer,timeinterval-tsElapseTimer);
//				m_eventWaiter.timedwait(timeinterval-tsElapseTimer);
//			}
//#endif
//		}

		lastTime = packet.timestamp;

		///�ͷ����֡
		release(packet);


		//		///AVͬ��
		int64_t lastVideoPts;
		int playDelay = getAvSyncManager().controlAudioPts(pcmpacket.timestamp,
					1,
					lastVideoPts);
//		JPLAYER_LOG_INFO("controlAudioPts audio:%lld  video:%lld  playDelay:%d",pcmpacket.timestamp,lastVideoPts,playDelay);
//		if (playDelay > 0)  ///��Ƶ��ǰ�ˣ��ȴ�һ��
//		{
//			m_eventWaiter.timedwait(playDelay);
//		}
//		else if (playDelay < 0)  ///��Ƶ��ǰ�ˣ������Ƶ���壬�ȴ�����ݣ������Ż�
//		{
//			///���ſ��
//			clearAllBuffer(lastVideoPts);
//		}

	}
	return 0;
}
#endif

bool AudioManager::encodeAudio(StreamPacket& packet)
{
	StreamPacket pcmpacket;
	pcmpacket.data = packet.data;
	pcmpacket.size = packet.size;

#ifdef G726
	return true;
#else
	return m_AACEncode.writePacket(pcmpacket,packet);
//	return m_opusEncoder.writePacket(pcmpacket,packet);
#endif

}


///����ý�����
int AudioManager::writeMediaPacket(const StreamPacket& packet)
{
#ifdef AUDIO_TEST
	return 0;
#endif
//	JPLAYER_LOG_INFO("%s|%s(): %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx %02hhx",
//			__FILE__, __FUNCTION__, packet.data[0], packet.data[1], packet.data[2], packet.data[3], packet.data[4], packet.data[5], packet.data[6]);
	m_lastpushtime = packet.timestamp;
	if(videoStreamMode == 1 || videoStreamMode == 3)
	{
		bool ok = push(packet,false,1);

//		JPLAYER_LOG_INFO("writeMediaPacket audio size:%d",size());
		if (!ok)
		{
			JPLAYER_LOG_ERROR("writeMediaPacket audio failed");
			(void)ok;
		}
		return 0;
	}
	else
	{
		int count = 5;

		///���Է������
		bool ok = false;
		do
		{
			ok = push(packet,false,1);
//			JPLAYER_LOG_INFO("push return audio :  %d total size:%d  data size :%d",ok,size(),packet.size);
			///����ɹ����˳�
			if (ok)
			{
				break;
			}

			usleep(600*1000);
//			m_eventWaiter.timedwait(600);
			if(size() == 0)            //clear all buffer
				return 0;
//			m_eventWaiter.timedwait(10);     //live 10

		} while (count--);


		///���Զ��Ҳ�Ų����룬��������֡�ˣ���֮ǰ��֡
		if (!ok)
		{
			JPLAYER_LOG_INFO("drop frame  audio \n");
			ok = push(packet,true,1);
		}
		return 0;
	}
}

void AudioManager::close()
{
	clearAllBuffer();

	m_hasPause = false;

	stopSound();

	JPLAYER_LOG_INFO("AudioManager uninit ok\n");
}


LONG AudioManager::playSound()
{
//	if (!m_openAudio)
//	{
//		int ret = BwAudioCreate(11025,1,16);
//		if (ret != 0)
//		{
//			JPLAYER_LOG_INFO("AudioManager BwAudioCreateSL  fail\n");
//			return JPLAYER_MSG_NO_INIT_FAIL;
//		}
//
//#ifndef OS_MAC
//
//		WebRtcNs_Create((NsHandle **)&m_pNsHandle);
//		if (m_pNsHandle != NULL)
//		{
//			WebRtcNs_Init((NsHandle *)m_pNsHandle, m_sampleRate);
//			WebRtcNs_set_policy((NsHandle *)m_pNsHandle, 1);
//		}
//
//#endif
//
//		m_openAudio = true;
//	}
//
//	if (!m_playSound)
//	{
//		int ret = BwAudioStart();
//
//		m_playSound = (ret == 0);
//	}

	m_openAudio = false;
	m_firstFrame = true;

	return m_playSound?JPLAYER_MSG_NORMAL:JPLAYER_MSG_NO_INIT_FAIL;
}

LONG AudioManager::stopSound()
{

	if (!m_openAudio)
	{
		return JPLAYER_MSG_NORMAL;
	}

	if (m_playSound)
	{
//		BwAudioStop();
//		m_playSound = false;
		_stop_speaker = 1; // stop it in run()
//		JPLAYER_LOG_INFO("BwAudioStop");
	}

#ifdef OS_MAC
if (m_openAudio)
{
	BwAudioDestroy();
	m_openAudio = false;
}
#endif	

	int64_t lastVideoPts;
	getAvSyncManager().controlAudioPts(0,1,lastVideoPts);

	return JPLAYER_MSG_NORMAL;

}

LONG AudioManager::setVolume(unsigned int  volume)
{
	if (!m_openAudio)
	{
		return JPLAYER_MSG_NORMAL;
	}

	int ret = BwAudioSetVolume((float)volume);
	return ret == 0?JPLAYER_MSG_NORMAL:JPLAYER_MSG_NO_INIT_FAIL;

}

LONG AudioManager::getVolume(unsigned int*  pVolume)
{
	if (!m_openAudio)
	{
		return JPLAYER_MSG_NORMAL;
	}

	float volume;
	int ret = BwAudioGetVolume(&volume);
	if (ret != 0)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	if (pVolume != NULL)
	{
		*pVolume = (unsigned int) volume;
	}
	return JPLAYER_MSG_NORMAL;

}



bool  AudioManager::pause(bool hasPause)
{
	m_hasPause = hasPause;
	return true;
}

bool  AudioManager::setScale(float scale)
{
	m_scale = scale;
	return true;
}

bool AudioManager::clearAllBuffer(int64_t lastAudioPts)
{
	if (lastAudioPts > 0)
	{
		StreamPacket packet;
		while (popTimeWait(packet,1))
		{
			release(packet);

			if (packet.timestamp >= lastAudioPts)
			{
				break;
			}
		}
	}
	else
	{
		releaseAllBuffer();
	}

//	JPLAYER_LOG_INFO("release all buffer audio :%d",size());

	return true;
}

int AudioManager::setAudioEncodePara(int sample_rate,int channel,int bitrates,int fps)
{
#ifndef G726
//	return m_opusEncoder.setPara(sample_rate,channel,bitrates,fps);
#endif

	return 0;
}

int AudioManager::setAudioDecodePara(int sample_rate,int channel,int bitrates)
{
	m_sampleRate = sample_rate;
	m_channel = channel;
#ifndef G726
//	return m_opusDecoder.setPara(sample_rate,channel);
#endif
	return 0;
}

void AudioManager::setVideoStreamMode(int p_Mode, int p_MediaBufferLength)
{
	videoStreamMode = p_Mode;
	m_SeekSuccess = false;
	startPts = 0;
	startTime = 0;
	lastTime = 0;
	m_DropStreamTime = 0;
	int64_t lastVideoPts;
	getAvSyncManager().controlAudioPts(0,1,lastVideoPts);


}

void  AudioManager::setCurrentTime(long p_CurrentTime)
{
	int64_t lastVideoPts;
	getAvSyncManager().controlAudioPts(0,1,lastVideoPts);
//	JPLAYER_LOG_INFO("controlAudioPts  video:%lld",lastVideoPts);
}

void AudioManager::seekSuccess()
{
	m_SeekSuccess = true;
}

