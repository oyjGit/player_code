#include "MP4Recorder.h"
#include "strHelper.h"
#include "LYCode.h"
#include "LogOutput.h"

CMP4Recorder::CMP4Recorder(unsigned int maxTimeLen) :thread(false), CCallBackMsg(nullptr)
, mRecordDataQue(nullptr)
, mRecordingFlag(false)
, mMP4File(MP4_INVALID_FILE_HANDLE)
, mVideoTrackId(MP4_INVALID_TRACK_ID)
, mAudioTrackId(MP4_INVALID_TRACK_ID)
, mTrack(0) //1:video,2:audio
, mSpsData(nullptr)
, mVideoStartTime(0)
, mVideoFrameCount(0)
, mVideoDurationTimeScale(0)
, mAudioSampleRate(0)
, mAudioChannels(0)
, mAudioFrameCount(0)
, mAudioDurationTimeScale(0)
, mAudioStartTime(0)
, mRecordTimeInterval(maxTimeLen)
, mCustomFileName(nullptr)
{
	memset(mFileDir, 0, sizeof(mFileDir));
	memset(&mPreVideoPacket, 0, sizeof(mPreVideoPacket));
	memset(&mPreAudioPacket, 0, sizeof(mPreAudioPacket));
}

CMP4Recorder::~CMP4Recorder()
{
	if (mSpsData)
	{
		delete mSpsData;
	}
	if (mPreVideoPacket.data)
	{
		delete mPreVideoPacket.data;
	}
	if (mPreAudioPacket.data)
	{
		delete mPreAudioPacket.data;
	}

}

int CMP4Recorder::recordStart(const char* dir, CQueBuffer* que)
{
	int ret = EOK;
	mRecordDataQue = que;
	strcpy(mFileDir, dir);
	char fileName[MAX_PATH] = { 0 };
	getFileName(fileName, MAX_PATH);
	ret = openFile(fileName);
	if (EOK != ret)
	{
		return ret;
	}
	if (mRecordingFlag)
	{
		recordStop();
	}
	mRecordingFlag = true;
	start();

	return ret;
}

int CMP4Recorder::recordStart(CQueBuffer* que, const char* fileName)
{
	int ret = EOK;
	ret = openFile(fileName);
	if (EOK != ret)
	{
		return ret;
	}
	mRecordDataQue = que;
	if (mRecordingFlag)
	{
		recordStop();
	}
	mRecordingFlag = true;
	start();

	return ret;
}

bool CMP4Recorder::recordStop(const char* fileName)
{
	if (mRecordingFlag)
	{
		mCustomFileName = const_cast<char*>(fileName);
		mRecordingFlag = false;
		waitfor();

		closeMP4File();
		mRecordDataQue = NULL;
	}
	return true;
}

void CMP4Recorder::setCallBackObjInterface(CPlayerCallBack* obj)
{
	setCallBackObj(obj);
}

int CMP4Recorder::setRecordMaxTime(unsigned int len)
{
	mRecordTimeInterval = len;
	if (mRecordTimeInterval > DEFAULT_MAX_RECORD_TIME)
	{
		mRecordTimeInterval = DEFAULT_MAX_RECORD_TIME;
		return EREOCRD_SET_TIMEVAL_FAILED;
	}
	return 0;
}

void CMP4Recorder::execute()
{
	bool gotKey = false;
	char* dataBuf = new char[MAX_BUF_SIZE];
	SMediaPacket packetImp;
	packetImp.bufSize = mRecordDataQue->getBufSize();
	packetImp.data = new char[packetImp.bufSize];
	SMediaPacket tmpPacket;
	SMediaPacket *packet = &packetImp;
	unsigned long startTime = timeGetTime();
	while (mRecordingFlag)
	{
		if (mRecordDataQue)
		{
			bool result = mRecordDataQue->pop(packetImp);
			if (!result)
			{
				Sleep(50);
				continue;
			}

			memset(dataBuf, 0, MAX_BUF_SIZE);
			memset(&tmpPacket, 0, sizeof(SMediaPacket));

			if (timeGetTime() > (startTime + mRecordTimeInterval))
			{
				recordFull2Again();
				startTime = timeGetTime();
			}

			switch (packet->frameType)
			{
			case 1:
			case 5:
			case 6:
			case 7:
			case 8:
			{
				int startCodeLen(4);
				int len(0);
				unsigned int frameSize(0);
				if (packet->frameType == 7)
				{
					char* sps = getSepFrame(packet->data, packet->size, 7, &startCodeLen, &len);
					if (sps)
					{
						//第一次收到sps
						if (NULL == mSpsData)
						{
							if (!addVideoTrack(sps + startCodeLen, len, packet->timestamp))
							{
								continue;
							}
						}
						setMP4VideoSPS(sps + startCodeLen, len);
						char* ptr = sps + startCodeLen + len;
						int left = packet->size - startCodeLen - len;
						char* pps = getSepFrame(ptr, left, 8, &startCodeLen, &len);
						if (pps)
						{
							setMP4VideoPPS(pps + startCodeLen, len);
							ptr = pps + startCodeLen + len;
							left -= startCodeLen + len;
							char* iFrame = getSepFrame(ptr, left, 5, &startCodeLen, &len);
							if (iFrame)
							{
								unsigned int frameLen = len;
								frameSize = htonl(frameLen);
								memcpy(dataBuf, &frameSize, sizeof(frameSize));
								memcpy(dataBuf + sizeof(frameSize), iFrame + startCodeLen, frameLen);
								tmpPacket.data = dataBuf;
								tmpPacket.size = frameLen + sizeof(frameSize);
								tmpPacket.timestamp = packet->timestamp;
								tmpPacket.frameType = 1;
								writeVideoData(&tmpPacket);
							}
						}
					}

				}
				else if (packet->frameType == 8)
				{
					char* pps = getSepFrame(packet->data, packet->size, 8, &startCodeLen, &len);
					if (pps)
					{
						setMP4VideoPPS(pps, len);
					}
				}
				else
				{
					findStartCode(packet->data, packet->size, &startCodeLen, NULL);
					unsigned int frameLen = packet->size - startCodeLen;
					frameSize = htonl(frameLen);
					memcpy(dataBuf, &frameSize, sizeof(frameSize));
					memcpy(dataBuf + sizeof(frameSize), packet->data + startCodeLen, frameLen);
					int writeSize = packet->size + sizeof(frameSize) - startCodeLen;
					tmpPacket.data = dataBuf;
					tmpPacket.size = writeSize;
					tmpPacket.timestamp = packet->timestamp;
					tmpPacket.frameType = (packet->frameType == 5);
					writeVideoData(&tmpPacket);
				}

			}
				break;
			case 9:
			case 127:
			case 128:
			case 129:
			{
				if (packet->size > 7)
				{
					if (mAudioSampleRate == 0)
					{
						if (!aacADTS2ACInfo(packet->data, packet->size))
						{
							CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "can't get adts info,mp4 record audio only support aac");
							continue;
						}
						addAudioTrack();
					}
					memcpy(&tmpPacket, packet, sizeof(packet));
					tmpPacket.data = packet->data + 7;
					tmpPacket.size = packet->size - 7;
					writeAudioData(&tmpPacket);
				}
			}
				break;
			default:
				CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "录像文件不支持的帧类型:%d,size=%d", packet->frameType, packet->size);
				break;
			}
		}
		else
		{
			Sleep(100);
			continue;
		}
	}
	closeMP4File();
	if (dataBuf)
	{
		delete dataBuf;
		dataBuf = NULL;
	}
	if (packetImp.data)
	{
		delete packetImp.data;
	}
}

void CMP4Recorder::getFileName(char* fileName, int len)
{
	if (fileName && len > 0)
	{
		char curTimeStr[128] = { 0 };
		memset(fileName, 0, len);
		strcpy(fileName, mFileDir);
		strcat(fileName, "record");
		char thisStr[24] = {0};
		sprintf(thisStr, "%x_", this);
		strcat(fileName,thisStr);
		getCurrentTimeStr(curTimeStr, 128);
		strcat(fileName, curTimeStr);
		strcat(fileName,".mp4");
	}
}

int CMP4Recorder::openFile(const char* fileName)
{
	if (NULL == fileName)
	{
		return EINVALID_PARAM;
	}
	//mMP4File = MP4CreateEx(fileName, 0, 1, 1, 0, 0, 0, 0);
	mMP4File = MP4Create(fileName);
	if (mMP4File == MP4_INVALID_FILE_HANDLE)
	{
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "创建mp4文件失败:%s", fileName);
		return EREOCRD_CREATE_FILE_FAILED;
	}
	MP4SetTimeScale(mMP4File, VIDEO_TIME_SCALE);

	return EOK;
}

bool CMP4Recorder::addVideoTrack(char* sps,int len,unsigned long long timeStamp)
{
	if (NULL == sps || len < 0)
	{
		return false;
	}
	if (mVideoTrackId == MP4_INVALID_TRACK_ID)
	{
		mSpsData = new h264_sps_data_t;
		h264_parse_sps((uint8_t*)sps + 1, len - 1, mSpsData);
		//添加h264 track 
		mVideoTrackId = MP4AddH264VideoTrack(mMP4File, VIDEO_TIME_SCALE, MP4_INVALID_DURATION,
			mSpsData->width,				//video width
			mSpsData->height,				//video height
			sps[1],							//sps[1] AVCProfileIndication 
			sps[2],							//profile_compat 
			sps[3],							//AVCLevelIndication 
			3);								//4 bytes length before each NAL unit 

		if (mVideoTrackId == MP4_INVALID_TRACK_ID)
		{
			//setCallBackMsg(0, "add video track failed", 0);
			return false;
		}
		MP4SetVideoProfileLevel(mMP4File, 0x7F);
		
		//mVideoStartTime = timeStamp;
		mTrack |= mVideoTrackId;//加入video track
		return true;
	}
	return false;
}

void CMP4Recorder::setMP4VideoSPS(char* sps, int spsLen)
{
	if (sps && spsLen > 0)
	{
		MP4AddH264SequenceParameterSet(mMP4File, mVideoTrackId, (unsigned char*)sps, spsLen);
	}
}

void CMP4Recorder::setMP4VideoPPS(char* pps, int ppsLen)
{
	if (pps && ppsLen > 0)
	{
		MP4AddH264PictureParameterSet(mMP4File, mVideoTrackId, (unsigned char*)pps, ppsLen);
	}
}

unsigned long long CMP4Recorder::getTimeScaleFromTicks(unsigned long long time, unsigned int timeScale)
{
	time *= (unsigned long long)timeScale;
	time /= 1000000;
	return time;
}

void CMP4Recorder::writeVideoData(SMediaPacket* packet)
{
	if (packet->data && packet->size > 0 && mVideoTrackId != MP4_INVALID_TRACK_ID)
	{
		if (mVideoFrameCount == 0)
		{
			if (packet->frameType)
			{
				mVideoStartTime = packet->timestamp;
				if (mPreVideoPacket.data == NULL)
				{
					mPreVideoPacket.data = new char[MAX_BUF_SIZE];
					mPreVideoPacket.bufSize = MAX_BUF_SIZE;
				}
				mPreVideoPacket.size = packet->size;
				mPreVideoPacket.frameType = packet->frameType;
				mPreVideoPacket.timestamp = packet->timestamp;
				memcpy(mPreVideoPacket.data, packet->data, packet->size);
				mVideoFrameCount++;
			}
			else
			{
				return;
			}
		}

		//获取当前帧和前一帧时间差，此时间主要作为音视频同步用
		unsigned long long timeTick = packet->timestamp - mVideoStartTime;
		unsigned long long timeInScaleTotal = 0;
		unsigned long long timeInScaleCurFrame = 0;
		if (timeTick < 0)
		{
			//setCallBackMsg(0, "record video time stamp roll back", 0);
			return;
		}
		timeTick *= 1000;
		timeInScaleTotal = getTimeScaleFromTicks(timeTick, VIDEO_TIME_SCALE);
		timeInScaleCurFrame = timeInScaleTotal - mVideoDurationTimeScale;
		mVideoDurationTimeScale += timeInScaleCurFrame;
		if (!MP4WriteSample(mMP4File, mVideoTrackId, (unsigned char*)mPreVideoPacket.data, mPreVideoPacket.size, timeInScaleCurFrame, 0, (mPreVideoPacket.frameType == 1)))
		{
			setCallBackMsg(EEID_RECORD_WRITE_V_FAILED, (void*)mVideoFrameCount, 0);
		}
		
		//填充下一次写的数据
		char* tmp = mPreVideoPacket.data;
		memset(&mPreVideoPacket, 0, sizeof(SMediaPacket));
		memcpy(&mPreVideoPacket,packet,sizeof(SMediaPacket));
		mPreVideoPacket.data = tmp;
		memcpy(mPreVideoPacket.data, packet->data, packet->size); 
		mVideoFrameCount++;
	}
}

bool CMP4Recorder::aacADTS2ACInfo(const char* adts, int len)
{
	if (adts && len >= 7)
	{
		if (((adts[0] & 0xff) == 0xff) && ((adts[0] & 0xf1) == 0xf1))
		{
			int sample[] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350 };
			int samIndex = ((unsigned char)adts[2] & 0x3c) >> 2;
			if (samIndex >= sizeof(sample) / sizeof(sample[0]))
			{
				return false;
			}
			mAudioSampleRate = sample[samIndex];
			mAudioChannels = (((unsigned char)adts[2] & 0x1) << 2) | (((unsigned char)adts[3] & 0xc0) >> 6);
			unsigned char profile = ((((unsigned char)adts[2]) & 0xc0) >> 6);

			mAudioAACInfo[0] = 0;
			mAudioAACInfo[1] = 0;

			mAudioAACInfo[0] = (profile << 3);
			mAudioAACInfo[0] |= samIndex >> 1;

			mAudioAACInfo[1] = (samIndex & 0x1 << 7);
			mAudioAACInfo[1] |= mAudioChannels << 3;
		}
	}
	return true;
}

bool CMP4Recorder::addAudioTrack()
{
	//if (mVideoTrackId == MP4_INVALID_TRACK_ID)
		//return false;
	if (mAudioTrackId != MP4_INVALID_TRACK_ID)
	{
		return true;
	}
	mAudioTrackId = MP4AddAudioTrack(mMP4File, mAudioSampleRate, MP4_INVALID_DURATION, MP4_MPEG4_AUDIO_TYPE);
	if (MP4_INVALID_TRACK_ID == mAudioTrackId)
	{
		//setCallBackMsg(0, "record add audio tracker failed", 0);
		return false;
	}
	int profile = (mAudioAACInfo[0] >> 3) & 0x1f;
	//MP4SetAudioProfileLevel(mMP4File, 0x02);
	MP4SetAudioProfileLevel(mMP4File, profile);
	MP4SetTrackESConfiguration(mMP4File,mAudioTrackId,mAudioAACInfo,2);
	mTrack |= mAudioTrackId;
	return true;
}

void CMP4Recorder::writeAudioData(SMediaPacket* packet)
{
	if (packet->data && packet->size > 0 && mAudioTrackId != MP4_INVALID_TRACK_ID)
	{
		if (mAudioFrameCount == 0)
		{
			if (mVideoFrameCount < 2)
			{
				return;
			}
			mAudioStartTime = packet->timestamp;
			if (mPreAudioPacket.data == NULL)
			{
				mPreAudioPacket.data = new char[MAX_BUF_SIZE];
				mPreAudioPacket.bufSize = MAX_BUF_SIZE;
			}
			mPreAudioPacket.size = packet->size;
			mPreAudioPacket.timestamp = packet->timestamp;
			memcpy(mPreAudioPacket.data, packet->data, packet->size);
			mAudioDurationTimeScale = 0;
			mAudioFrameCount++;
			return;
		}

		//获取当前帧和前一帧时间差，此时间主要作为音视频同步用
		unsigned long long timeTick = packet->timestamp - mAudioStartTime;
		unsigned long long timeInScaleTotal = 0;
		unsigned long long timeInScaleCurFrame = 0;
		if (timeTick < 0)
		{
			//setCallBackMsg(0, "record audio time stamp roll back", 0);
			return;
		}
		timeTick *= 1000;
		timeInScaleTotal = getTimeScaleFromTicks(timeTick, mAudioSampleRate);
		timeInScaleCurFrame = timeInScaleTotal - mAudioDurationTimeScale;
		mAudioDurationTimeScale += timeInScaleCurFrame;
		if (!MP4WriteSample(mMP4File, mAudioTrackId, (unsigned char*)mPreAudioPacket.data, mPreAudioPacket.size, timeInScaleCurFrame, 0, false))
		{
			setCallBackMsg(EEID_RECORD_WRITE_A_FAILED, (void*)mAudioFrameCount, 0);
		}
		
		//填充下一次写的数据
		char* tmp = mPreAudioPacket.data;
		memset(&mPreAudioPacket, 0, sizeof(SMediaPacket));
		memcpy(&mPreAudioPacket, packet, sizeof(SMediaPacket));
		mPreAudioPacket.data = tmp;
		memcpy(mPreAudioPacket.data, packet->data, packet->size);
		mAudioFrameCount++;
	}
}

int CMP4Recorder::recordFull2Again()
{
	closeMP4File();
	int ret = EOK;
	char fileName[MAX_PATH] = { 0 };
	getFileName(fileName, MAX_PATH);
	ret = openFile(fileName);
	if (EOK != ret)
	{
		return ret;
	}
	return ret;
}

void CMP4Recorder::closeMP4File()
{
	if (mMP4File != MP4_INVALID_FILE_HANDLE)
	{
		writeVideoData(&mPreVideoPacket);
		writeAudioData(&mPreAudioPacket);
		const char* name = MP4GetFilename(mMP4File);
		char fileTmp[MAX_PATH] = { 0 };
		strncpy(fileTmp, name, MAX_PATH - 1);
		fileTmp[MAX_PATH - 1] = '\0';
		//参数：hFile 文件句柄，flags 是否允许在关闭MP4文件前做一些额外的优化处理。
		//注意：在录制较小的MP4文件时可以把flags设置为默认值，如果录制较大的文件最好把flags设置为MP4_CLOSE_DO_NOT_COMPUTE_BITRATE否则调用MP4Close函数会用掉很长的时间。
		MP4Close(mMP4File);
		mMP4File = MP4_INVALID_FILE_HANDLE;
		MP4Optimize(fileTmp);
		if (mCustomFileName != nullptr && strlen(mCustomFileName) != 0)
		{
			char newName[MAX_PATH] = { 0 };
			strncpy(newName, mFileDir, MAX_PATH - 1);
			int left = MAX_PATH - strlen(mFileDir);
			strncat(newName, mCustomFileName, left - 1);
			if (strstr(newName, ".mp4") == NULL)
			{
				left = MAX_PATH - strlen(newName);
				if (left > 5)
					strncat(newName, ".mp4", left);
				else
				{
					newName[MAX_PATH - 2] = '4';
					newName[MAX_PATH - 3] = 'p';
					newName[MAX_PATH - 4] = 'm';
					newName[MAX_PATH - 5] = '.';
				}
			}
			newName[MAX_PATH - 1] = '\0';
			if (rename(fileTmp, newName) != 0)
			{
				setCallBackMsg(EEID_REOCRD_RENAME_FAILED, (void*)GetLastError(), 0);
			}
		}

		if (mSpsData)
		{
			delete mSpsData;
			mSpsData = NULL;
		}
		if (mPreVideoPacket.data)
		{
			delete mPreVideoPacket.data;
			mPreVideoPacket.data = NULL;
		}
		if (mPreAudioPacket.data)
		{
			delete mPreAudioPacket.data;
			mPreAudioPacket.data = NULL;
		}

		mTrack = 0;
		mVideoDurationTimeScale = 0;
		mVideoFrameCount = 0;
		mVideoStartTime = 0;
		mVideoTrackId = MP4_INVALID_TRACK_ID;

		mAudioTrackId = MP4_INVALID_TRACK_ID;
		mAudioSampleRate = 0;
		mAudioChannels = 0;
		mAudioDurationTimeScale = 0;
		mAudioFrameCount = 0;
		mAudioStartTime = 0;
		mAudioAACInfo[0] = 0;
		mAudioAACInfo[1] = 0;
		mCustomFileName = nullptr;
	}
}