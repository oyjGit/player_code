#ifndef __LY_RECORDER_FACTORY_H__
#define __LY_RECORDER_FACTORY_H__

#include "PlayerTypedef.h"
#include "IMediaRecorder.h"

/*******************************************************************************
*Copyright (c) 2016, Visual Studio
*Allrights reserved.
*
*@file    MP4Recorder.h
*@brief   羚羊云windows播放器录像工厂类。
*@author  owen
*@date    2016-08-11
*@version 1.0
修订说明：
******************************************************************************/

class CRecorderFactory
{
public:
	static IMediaRecorder* createRecorder(ERecordFormat type = ERECORD_FORMAT_MP4);
public:
	CRecorderFactory(void);
	~CRecorderFactory(void);
};

#endif