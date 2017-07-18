#ifndef AV_SYNC_H_
#define AV_SYNC_H_

#include "MediaInfo.h"


typedef enum
{
	AV_SYNC_AUDIO = 0,
	AV_SYNC_VIDEO = 1,

	AV_SYNC_EXTERN
}JPlayer_AVSync_Mode;



class AvSyncManager
{
public:
	AvSyncManager();
	virtual ~AvSyncManager();

	inline bool setAvSyncMode(int mode)
	{
		m_syncMode = mode;
		return true;
	}

	inline bool setLastVideoPts(int64_t lastVideoPts)
	{
		m_lastVideoPts = lastVideoPts;
		return true;
	}

	inline bool setLastAudioPts(int64_t lastAudioPts)
	{
		m_lastAudioPts = lastAudioPts;
		return true;
	}

	///0��>0�ӳ�ʱ��,-1�ⲿ��Ҫ����
	int controlVideoPts(int64_t lastVideoPts,
		int oneDisplayTime,
		int64_t &lastAudioPts,int & hasAudioFlag);
	int controlAudioPts(int64_t lastAudioPts,
		int onePlayTime,
		int64_t &lastVideoPts);

	void setCurrentDelayTime(int p_DelayTime);
	int getCurrentDelayTime();
	int controlPlayerDelaytime(const int p_PlayDelay);
	int dropPlayerDelaytime(const int p_DeltaTime);

	void setAudioDefaultDropTime();
	void setVideoDefaultDropTime();

	void setDropTime(int aDropTime,int aDropType);//aDropType:0 正常丢帧   1 校准基准帧
	int getDropType();

	int getVideoDefaultDropTime();
	int getAudioDefaultDropTime();

	int getCalculateTime();
	void controlDefaultDropTime(const int p_MinDelayTime, const int p_MaxDelayTime);

	void setBeginTime(long p_BeginTime);

	void setCurrentTime(long p_CurrentTime);

	int getFullBufferTime(const int p_FullBufferTime);

	inline int64_t getLastVideoPts()
	{
		return m_lastVideoPts;
	}

private:
	unsigned int Sys_GetTickCount1(void);

private:
	int64_t m_firstVideoPts;
	int64_t m_lastVideoPts;
	int64_t m_lastAudioPts;

	int m_syncMode;

	int m_avDiffMinValue;
	int m_avDiffMaxValue;

	int m_videoFps;

	long m_CurrentPosition;
	long m_BeginPosition;

	long m_PreviousBaseTime;

	int m_firsttime;

	int m_DelayTime;

	int m_lastDeltaTime;    //缓冲区首尾两帧时间戳插值

	int m_totalAudioDropTime;
	int m_totalVideoDropTime;

	int m_MinDelayTime;

	int m_MaxDelayTime;

	int m_HasAudioFlag;

	int m_DropType;

	unsigned int lastSysTime;

	unsigned int lastAudioUpdateTime;
};



/////////////////////////////////////////////////////////////////////////////

#endif /* AV_SYNC_H_ */





