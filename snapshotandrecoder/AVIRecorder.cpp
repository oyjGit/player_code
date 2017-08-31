#include "AVIRecorder.h"

CAVIRecorder::CAVIRecorder() :thread(false),CCallBackMsg(NULL)
, mRecordDataQue(NULL)
, mRecordingFlag(NULL)
{

}

CAVIRecorder::~CAVIRecorder()
{

}

int CAVIRecorder::recordStart(const char* dir, CQueBuffer* que)
{
	return 0;
}

int CAVIRecorder::recordStart(CQueBuffer* que, const char* fileName)
{
	return 0;
}

bool CAVIRecorder::recordStop(const char* fileName)
{
	return true;
}

void CAVIRecorder::setCallBackObjInterface(CPlayerCallBack* obj)
{
	setCallBackObj(obj);
}

int CAVIRecorder::setRecordMaxTime(unsigned int len)
{
	return 0;
}

void CAVIRecorder::execute()
{

}