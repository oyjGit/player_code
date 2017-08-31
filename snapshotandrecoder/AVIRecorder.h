#ifndef __LY_AVI_RECORDER_H__
#define __LY_AVI_RECORDER_H__

/*******************************************************************************
*Copyright (c) 2016, Visual Studio
*Allrights reserved.
*
*@file    MP4Recorder.h
*@brief   ÁçÑòÔÆwindows²¥·ÅÆ÷aviÂ¼ÏñÀà¡£
*@author  owen
*@date    2016-08-11
*@version 1.0
ÐÞ¶©ËµÃ÷£º
******************************************************************************/

#include "IMediaRecorder.h"
#include "PlayerCallBack.h"
#include "avilib.h"
#include "QueBuffer.h"
#include "pasync.h"

class CAVIRecorder:public IMediaRecorder,public pt::thread,public CCallBackMsg
{
public:
	CAVIRecorder();
	~CAVIRecorder();
	virtual int recordStart(const char* dir,CQueBuffer* que);
	virtual int recordStart(CQueBuffer* que,const char* fileName);
	virtual bool recordStop(const char* fileName = nullptr);
	void setCallBackObjInterface(CPlayerCallBack* obj);
	int setRecordMaxTime(unsigned int len);
protected:
	void execute();
private:
	CQueBuffer*			mRecordDataQue;
	bool*				mRecordingFlag;
	char				mFileDir[MAX_PATH];
};
#endif