#ifdef WIN32
#include <Windows.h>
#endif
 #include <unistd.h>

#include "AvSync.h"
#include "Logs.h"

AvSyncManager::AvSyncManager()
	:m_lastVideoPts(0),
	 m_lastAudioPts(0),
	 m_syncMode(AV_SYNC_AUDIO),
	 m_avDiffMinValue(100),
	 m_avDiffMaxValue(2000),
	 m_videoFps(10),
	 m_DelayTime(100),
	 m_totalAudioDropTime(0),
	 m_totalVideoDropTime(0),
	 m_MinDelayTime(100),
	 m_MaxDelayTime(5000),
	 m_HasAudioFlag(0),
	 m_DropType(0),
	 lastSysTime(0),
	 lastAudioUpdateTime(0)
{

}

AvSyncManager::~AvSyncManager()
{
}

unsigned int AvSyncManager::Sys_GetTickCount1(void)
{
	struct timeval aika;
	gettimeofday(&aika,NULL);
	unsigned long msecs = aika.tv_sec * 1000;
	msecs += (aika.tv_usec / 1000);
	return msecs;
}

int AvSyncManager::controlVideoPts(int64_t lastVideoPts,
								   int oneDisplayTime,
								   int64_t &lastAudioPts,int & hasAudioFlag)
{

	///都以毫秒为单位
//	int delayTime = (1000 / m_videoFps) - oneDisplayTime - 1 ;
//	unsigned long long ldelayTime = lastVideoPts - m_lastVideoPts - oneDisplayTime - 1 ;
//
//	int delayTime;
//	if(m_lastVideoPts == 0)
//		delayTime = (1000 / m_videoFps) - oneDisplayTime - 1 ;
//	else
//		delayTime = lastVideoPts - m_lastVideoPts - oneDisplayTime - 1 ;
//
//	JPLAYER_LOG_INFO("delayTime :%d ",delayTime);

	int videoDeltaPts = 0;
	if(m_lastVideoPts != 0)
		videoDeltaPts = lastVideoPts - m_lastVideoPts;

	int64_t nLastVideoPts = m_lastVideoPts;
	setLastVideoPts(lastVideoPts);

	lastAudioPts = m_lastAudioPts;

	///以视频为轴播放，视频自由播放
	if (m_syncMode == AV_SYNC_VIDEO)
	{
		return 0;
	}

	hasAudioFlag = 1;


//	///都以毫秒为单位
//	int delayTime = (1000 / m_videoFps) - oneDisplayTime - 1 ;

	int delayTime = 0;

	int avDiffer = m_lastAudioPts - m_lastVideoPts;

//	JPLAYER_LOG_INFO("avDiffer :%d  %llu %llu",avDiffer,m_lastAudioPts,m_lastVideoPts);

//	JPLAYER_LOG_INFO("nDeltaPts 000  %llu %llu",m_lastVideoPts,nLastVideoPts);

//	static int i= 0;
//	i++;

	unsigned int currentSysTime = Sys_GetTickCount1();

	if(m_lastAudioPts == 0 || (currentSysTime - lastAudioUpdateTime) > 2000)
	{
//		JPLAYER_LOG_INFO("no audio");
		m_HasAudioFlag = 0;
		hasAudioFlag = 0;
		delayTime = 20;

		int nDeltaPts =  (m_lastVideoPts - nLastVideoPts)- (currentSysTime - lastSysTime);
		if(lastSysTime>0 && nDeltaPts>0 && nLastVideoPts>0 && m_lastVideoPts!=nLastVideoPts)
		{
			JPLAYER_LOG_INFO("nDeltaPts %d  %llu %llu ",nDeltaPts,m_lastVideoPts,nLastVideoPts);
			lastSysTime = currentSysTime;
			return nDeltaPts;
		}
		lastSysTime = currentSysTime;

		return 0;
	}

	m_HasAudioFlag = 1;

	if(avDiffer >= 0)
	{
		if(avDiffer > m_avDiffMinValue)
		{
//			JPLAYER_LOG_INFO("avDiffer 000000");
			delayTime = 20 < (videoDeltaPts/2) ? 20:(videoDeltaPts/2);
		}
		else
		{
//			JPLAYER_LOG_INFO("avDiffer 111111");
			delayTime = 40 < (videoDeltaPts*2/3) ? 40:(videoDeltaPts*2/3);
//			delayTime = avDiffer;
		}
	}
	else if(-avDiffer >= m_avDiffMaxValue)
	{
//		JPLAYER_LOG_INFO("avDiffer 22222");
//		hasAudioFlag = 0;
//		delayTime = 50;

		delayTime = m_avDiffMaxValue;
//		delayTime = -avDiffer/2;
	}
	else if(-avDiffer >= 200)
	{
//		JPLAYER_LOG_INFO("avDiffer 333333");

//		if(m_lastAudioPts == 0)
//			delayTime = 50;
//		else
			delayTime = -avDiffer;
	}
	else
	{
//		JPLAYER_LOG_INFO("avDiffer 444444");
		delayTime = -avDiffer;
	}

//	///音频快
//	if (avDiffer >  m_avDiffMinValue)
//	{
//		///音频快到一定范围，视频加速播放
////		if (avDiffer < m_avDiffMaxValue)
////		{
////			delayTime -= delayTime/3;
////		}
////		else  ///音频太快了，视频尽可能加速播放
////		{
////			delayTime -= delayTime/2;
////		}
//
//		JPLAYER_LOG_INFO("delayTime  :20");
//
//		delayTime = 20;
//
//	}///视频快
//	else if (avDiffer <  0-m_avDiffMinValue)
//	{
//		///视频快到一定范围，视频减速播放
////		if (avDiffer > 0-m_avDiffMaxValue)
////		{
////			delayTime += delayTime/3;
////		}
////		else ///视频太快了，视频尽可能减速播放
////		{
////			delayTime += delayTime/2;
////		}
//
//		delayTime = m_avDiffMinValue;
//
//		JPLAYER_LOG_INFO("delayTime m_avDiffMinValue :%d",m_avDiffMinValue);
//
//
////		JPLAYER_LOG_INFO("controlVideoPts:avDiffer=== %lld delayTime === %d\n",
////			avDiffer,delayTime);
//	}
//	else if(avDiffer <  m_avDiffMinValue)
//		delayTime = avDiffer;


	if (delayTime <= 0)
	{
		///不让外部丢帧
		delayTime = 0;
	}

    return delayTime;
}



int AvSyncManager::controlAudioPts(int64_t lastAudioPts,
								   int onePlayTime,
								   int64_t &lastVideoPts)
{
	setLastAudioPts(lastAudioPts);

	lastAudioUpdateTime = Sys_GetTickCount1();

	lastVideoPts = m_lastVideoPts;

	///以音频为轴播放，音频自由播放
	if (m_syncMode == AV_SYNC_AUDIO)
	{
		return 0;
	}

	///都以毫秒为单位
	int delayTime = 1000/25 - onePlayTime - 1;

	int64_t avDiffer = m_lastVideoPts - m_lastAudioPts;
	///视频超前了
	if (avDiffer >  m_avDiffMinValue)
	{
		///告诉外部丢帧
		return -1;
	}///音频超前了
	else if (avDiffer <  -m_avDiffMinValue)
	{
		if (avDiffer > -m_avDiffMaxValue)
		{
			delayTime += delayTime/4;
		}
		else
		{
			delayTime += delayTime/2;
		}
	}
	else
	{
		///自由播放
		return 0;
	}


	return delayTime;

}

void AvSyncManager::setCurrentDelayTime(int p_DelayTime)
{
//	m_DelayTime = p_DelayTime;
	m_DelayTime = p_DelayTime > m_MinDelayTime?p_DelayTime:m_MinDelayTime;
//	m_DelayTime = m_DelayTime < m_MaxDelayTime?m_DelayTime:m_MaxDelayTime;
}

int AvSyncManager::getCurrentDelayTime()
{
	return m_DelayTime;
}

int AvSyncManager::controlPlayerDelaytime(const int p_PlayDelay)
{
	int nDiffDelay = p_PlayDelay - m_DelayTime;
	int tempDelayTime = m_DelayTime;
	if(nDiffDelay > 0)
	{
		if(nDiffDelay <= 500)
		{
			if(nDiffDelay > 100)
			{
//				tempDelayTime += (nDiffDelay/100*100);
				tempDelayTime = tempDelayTime>m_MaxDelayTime?m_MaxDelayTime:tempDelayTime;
	//			JPLAYER_LOG_INFO("tempDelayTime :%d",tempDelayTime);
				return tempDelayTime;
			}
		}
		else
		{
			tempDelayTime += 500;
			tempDelayTime = tempDelayTime>m_MaxDelayTime?m_MaxDelayTime:tempDelayTime;
//			JPLAYER_LOG_INFO("tempDelayTime :%d",tempDelayTime);
			return tempDelayTime;
		}
	}
	return tempDelayTime;
}

int AvSyncManager::getFullBufferTime(const int p_FullBufferTime)
{
//	if(m_DelayTime > m_MinDelayTime + 1000)
//	{
//		if(p_FullBufferTime > m_MinDelayTime + 500)
//			return p_FullBufferTime-m_MinDelayTime;
//		else if(p_FullBufferTime > m_MinDelayTime)
//			return p_FullBufferTime-m_MinDelayTime;
//		else
//			return -1;
//	}
//	else
//	{
//		if(p_FullBufferTime > m_MinDelayTime)
//			return p_FullBufferTime-m_MinDelayTime;
//		else
//			return -1;
//	}

	if(p_FullBufferTime > m_MinDelayTime)
		return p_FullBufferTime-m_MinDelayTime;
	else
		return -1;
}


int AvSyncManager::dropPlayerDelaytime(const int p_DeltaTime)
{
//	if(p_DeltaTime > m_MinDelayTime+1000)
//	{
//		if(m_DelayTime > m_MinDelayTime+1500)
//		{
//			m_totalDropTime += 500;
//			m_DelayTime -= 500;
//		}
//		else
//		{
//			int deltatime = 0;
//			deltatime = (m_DelayTime - m_MinDelayTime) > 100?100:(m_DelayTime - m_MinDelayTime);
//			m_DelayTime -= deltatime;
//			m_totalDropTime+=deltatime;
//		}
//	}
//	else if(p_DeltaTime >100)
//	{
//		int deltatime = 0;
//		deltatime = (m_DelayTime - m_MinDelayTime) > 100?100:(m_DelayTime - m_MinDelayTime);
//		m_DelayTime -= deltatime;
//		m_totalDropTime+=deltatime;
//	}
//	return m_DelayTime;

	int delay = m_DelayTime;
	if(p_DeltaTime > 1000)
	{
		delay = (m_DelayTime - 800) > m_MinDelayTime?(m_DelayTime - 800):m_MinDelayTime;
	}
	else if(p_DeltaTime > 500)
	{
		delay = (m_DelayTime - 400) > m_MinDelayTime?(m_DelayTime - 400):m_MinDelayTime;
	}
	else if(p_DeltaTime > 100)
	{
		delay = (m_DelayTime - p_DeltaTime/100*100) > m_MinDelayTime?(m_DelayTime - p_DeltaTime/100*100):m_MinDelayTime;
	}

	if(m_DelayTime != delay)
	{
		m_totalAudioDropTime += (m_DelayTime - delay);
		m_totalVideoDropTime += (m_DelayTime - delay);
		m_DelayTime = delay;
	}
	return m_DelayTime;
}

int AvSyncManager::getCalculateTime()
{
//	if(m_DelayTime >= 5000)
//		return 5000;
//	else if(m_DelayTime >= 4000)
//		return 6000;
//	else if(m_DelayTime >= 3000)
//		return 7000;
//	else if(m_DelayTime >= 2000)
//		return 8000;
//	else if(m_DelayTime >= 1000)
//		return 9000;
//	else if(m_DelayTime >= 500)
//		return 15000;
//	else if(m_DelayTime >= 100)
//		return 30000;
//	else
//		return 60000;

	if(m_DelayTime > m_MaxDelayTime/2)
		return 30000;
	else if(m_DelayTime <= m_MaxDelayTime/2 && m_DelayTime > m_MinDelayTime)
		return 60000;
	else
		return 100000000;
}

int AvSyncManager::getVideoDefaultDropTime()
{
	return m_totalVideoDropTime;
}

int AvSyncManager::getAudioDefaultDropTime()
{
	return m_totalAudioDropTime;
}

int AvSyncManager::getDropType()
{
	return m_DropType;
}

void AvSyncManager::setVideoDefaultDropTime()
{
	m_totalVideoDropTime = 0;
}

void AvSyncManager::setAudioDefaultDropTime()
{
	m_totalAudioDropTime = 0;
}

void AvSyncManager::setDropTime(int aDropTime,int aDropType)
{
    m_totalVideoDropTime = aDropTime;
    m_totalAudioDropTime = aDropTime;
    m_DropType = aDropType;
}

void AvSyncManager::controlDefaultDropTime(const int p_MinDelayTime, const int p_MaxDelayTime)
{
	m_totalAudioDropTime = 0;
	m_totalVideoDropTime = 0;
	m_DelayTime = p_MinDelayTime;
	m_MinDelayTime = p_MinDelayTime;
	m_MaxDelayTime = p_MaxDelayTime;
	m_HasAudioFlag = 0;
	lastSysTime = 0;
	lastAudioUpdateTime = 0;
}



