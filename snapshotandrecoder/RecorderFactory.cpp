#include "StdAfx.h"
#include "RecorderFactory.h"
#include "AVIRecorder.h"
#include "MP4Recorder.h"


CRecorderFactory::CRecorderFactory(void)
{

}

CRecorderFactory::~CRecorderFactory(void)
{

}

IMediaRecorder* CRecorderFactory::createRecorder(ERecordFormat type)
{
	IMediaRecorder*  recorder = NULL;
	switch (type)
	{
	case ERECORD_FORMAT_AVI:
		recorder = new CAVIRecorder;
		break;
	case ERECORD_FORMAT_MP4:
		recorder = new CMP4Recorder;
		break;
	default:
		break;
	}
	return recorder;
}