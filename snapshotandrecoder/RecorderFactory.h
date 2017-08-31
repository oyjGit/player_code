#ifndef __LY_RECORDER_FACTORY_H__
#define __LY_RECORDER_FACTORY_H__

#include "PlayerTypedef.h"
#include "IMediaRecorder.h"

/*******************************************************************************
*Copyright (c) 2016, Visual Studio
*Allrights reserved.
*
*@file    MP4Recorder.h
*@brief   ������windows������¼�񹤳��ࡣ
*@author  owen
*@date    2016-08-11
*@version 1.0
�޶�˵����
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