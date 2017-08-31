#ifndef __LY_MP4_RECORDER_H__
#define __LY_MP4_RECORDER_H__

/*******************************************************************************
*Copyright (c) 2016, Visual Studio
*Allrights reserved.
*
*@file    MP4Recorder.h
*@brief   ÁçÑòÔÆwindows²¥·ÅÆ÷mp4Â¼ÏñÀà¡£
*@author  owen
*@date    2016-08-11
*@version 1.0
ÐÞ¶©ËµÃ÷£º
******************************************************************************/

#include "IMediaRecorder.h"
#include "PlayerCallBack.h"
#include "QueBuffer.h"
#include "pasync.h"
#include "mp4v2/mp4v2.h"
#include "h264Parser.h"

#define VIDEO_TIME_SCALE 9000
#define DEFAULT_MAX_RECORD_TIME 0xffffe0 //ms

class CMP4Recorder:public IMediaRecorder,public pt::thread,public CCallBackMsg
{
public:
	CMP4Recorder(unsigned int timeLen = DEFAULT_MAX_RECORD_TIME);
	~CMP4Recorder();
	virtual int recordStart(const char* dir, CQueBuffer* que);
	virtual int recordStart(CQueBuffer* que, const char* dir);
	virtual bool recordStop(const char* fileName = nullptr);
	void setCallBackObjInterface(CPlayerCallBack* obj);
	int setRecordMaxTime(unsigned int len);
protected:
	void execute();
private:
	int recordFull2Again();
	void getFileName(char* fileName, int len);
	int openFile(const char* fileName);
	unsigned long long getTimeScaleFromTicks(unsigned long long time,unsigned int timeScale);
	//h264ÊÓÆµ²Ù×÷º¯Êý
	bool addVideoTrack(char* sps,int spsLen,unsigned long long timeStamp);
	void setMP4VideoSPS(char* sps,int spsLen);
	void setMP4VideoPPS(char* pps, int ppsLen);
	void writeVideoData(SMediaPacket* packet);
	//aacÒôÆµ²Ù×÷º¯Êý
	bool addAudioTrack();
	void writeAudioData(SMediaPacket* packet);
	bool aacADTS2ACInfo(const char* adts,int len);

	void closeMP4File();
private:
	CQueBuffer*			mRecordDataQue;
	bool				mRecordingFlag;
	char				mFileDir[MAX_PATH];
	MP4FileHandle		mMP4File;
	MP4TrackId			mVideoTrackId;
	MP4TrackId			mAudioTrackId;
	int					mTrack;

	SMediaPacket		mPreVideoPacket;
	h264_sps_data_t*	mSpsData;
	unsigned long long	mVideoStartTime;
	unsigned long long	mVideoFrameCount;
	unsigned long long	mVideoDurationTimeScale;

	SMediaPacket		mPreAudioPacket;
	int					mAudioSampleRate;
	int					mAudioChannels;
	int					mAudioFrameCount;
	unsigned long long	mAudioStartTime;
	unsigned long long	mAudioDurationTimeScale;
	unsigned char		mAudioAACInfo[2];

	pt::thread*			mReadThread;
	unsigned int		mRecordTimeInterval;
	char*				mCustomFileName;
	
};
#endif