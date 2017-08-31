#ifndef __LY_I_MEDIA_RECORDER_H__
#define __LY_I_MEDIA_RECORDER_H__
/*******************************************************************************
*Copyright (c) 2016, Visual Studio
*Allrights reserved.
*
*@file    MP4Recorder.h
*@brief   ������windows������¼��ӿ��ࡣ
*@author  owen
*@date    2016-08-11
*@version 1.0
�޶�˵����
******************************************************************************/

class CQueBuffer;
class CPlayerCallBack;
namespace pt{ class thread; }

class IMediaRecorder
{
public:
	virtual int recordStart(const char* dir, CQueBuffer* que) = 0;
	virtual int recordStart(CQueBuffer* que, const char* fileName) = 0;
	virtual bool recordStop(const char* fileName = nullptr) = 0;
	virtual void setCallBackObjInterface(CPlayerCallBack* obj){};
	virtual int setRecordMaxTime(unsigned int timeLen){ return 0; };
public:
	IMediaRecorder(void){};
	virtual ~IMediaRecorder(void){};
};

#endif