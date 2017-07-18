#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif

#include "ISCMSubscriberClient.h"
#include "JPlayer.h"
#include "TStringUtil.h"
#include "Logs.h"
#include "PlayerConfig.h"
#include "GlobalFunc.h"
#include "TTime.h"


#define  MAX_VIDEO_FRAME_LENGTH (800 * 1024)

JPlayer::JPlayer(const JPlayer_PlaySession &sess)
	:m_pSubscriberClient(NULL),
	 m_sess(sess),
	 m_jpMsgCallback(NULL),
	 m_msgDwUser(0),
	 m_videoWidth(0)
{

}

JPlayer::~JPlayer()
{
	stopAgent();

	closeAgent();

	notifyMediaStream(JPLAYER_MSG_SESSION_CLOSED,0,0);
}



LONG JPlayer::getLastError()
{
	if (m_pSubscriberClient == NULL)
	{
		return 0;
	}
	return m_pSubscriberClient->getLastError();
}

LONG JPlayer::setMsgCallBack(JPlayerMsgCallBack jpMsgCallback,LONG dwUser)
{
	m_jpMsgCallback = jpMsgCallback;
	m_msgDwUser = dwUser;

	getVideoManager().setMsgCallBack(m_jpMsgCallback,m_msgDwUser,(LONG)this);
	return JPLAYER_MSG_NORMAL;
}




LONG JPlayer::openAgent()
{
	if (!getVideoManager().open())
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}


	if (!getAudioManager().open())
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	if (!m_recorder.initRecorder())
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

    return JPLAYER_MSG_NORMAL;
}


LONG JPlayer::closeAgent()
{ 
	getVideoManager().close();

	getAudioManager().close();

	stopRecord();

	m_recorder.unInitRecorder();

	return JPLAYER_MSG_NORMAL;
}

LONG JPlayer::setPlaySession(const JPlayer_PlaySession &sess)
{
	stopAgent();

	m_sess = sess;

	return playAgent();
}

ISCMSubscriberClient *p_ISCMSubscriberClient;

LONG JPlayer::playAgent( int p_StreamMode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength)
{
	if(p_StreamMode == 1)
	{
		getVideoManager().setVideoStreamMode(1, p_MinMediaBufferLength, p_MaxMediaBufferLength);
		getAudioManager().setVideoStreamMode(1, p_MinMediaBufferLength);
	}
	else if(p_StreamMode == 2)
	{
		getVideoManager().setVideoStreamMode(2, p_MinMediaBufferLength, p_MaxMediaBufferLength);
		getAudioManager().setVideoStreamMode(2, p_MinMediaBufferLength);
	}
	else
	{
		getVideoManager().setVideoStreamMode(3, p_MinMediaBufferLength, p_MaxMediaBufferLength);
		getAudioManager().setVideoStreamMode(3, p_MinMediaBufferLength);
	}
	if(m_pSubscriberClient == NULL)
	{
		m_pSubscriberClient = new(std::nothrow) ISCMSubscriberClient(m_sess,this);
		p_ISCMSubscriberClient = m_pSubscriberClient;
	}
	m_pSubscriberClient->setStreamPath(p_StreamMode);
	if (m_pSubscriberClient == NULL)
	{
		JPLAYER_LOG_ERROR("playAgent m_pSubscriberClient is null\n");
		return JPLAYER_MSG_SUBSCRIBE_FAILED;
	}

	bool ok = m_pSubscriberClient->playLive();
	if (!ok)
	{
		JPLAYER_LOG_INFO("playAgent live error\n");
		return JPLAYER_MSG_SUBSCRIBE_FAILED;
	}

	return JPLAYER_MSG_NORMAL;
}

LONG JPlayer::playAgent()
{
	m_pSubscriberClient = new(std::nothrow) ISCMSubscriberClient(m_sess,this);
	if (m_pSubscriberClient == NULL)
	{
		return JPLAYER_MSG_SUBSCRIBE_FAILED;
	}
	
	bool ok = m_pSubscriberClient->playLive();
	if (!ok)
	{
		return JPLAYER_MSG_SUBSCRIBE_FAILED;
	}
	
	return JPLAYER_MSG_NORMAL;   
}


LONG JPlayer::playAgent(const JPlayer_RecordItem &item)
{
	return JPLAYER_MSG_NO_INIT_FAIL;
	
}


LONG JPlayer::stopAgent()
{
	if (m_pSubscriberClient != NULL)
	{
//		m_pSubscriberClient->stopLive();

		JPLAYER_LOG_INFO("111111");

		delete m_pSubscriberClient;
		m_pSubscriberClient = NULL;

		JPLAYER_LOG_INFO("222222");
	}

    return JPLAYER_MSG_NORMAL;
}

int splitString(const std::string &str,JPlayer_QueryRecordResponse& resp)
{
	std::vector<std::string> firstSplit;

	int countRsp = resp.m_sum;
	resp.m_sum = 0;

	size_t ret = comn::StringUtil::split(str,',',firstSplit);
	for (size_t i = 0; i < firstSplit.size(); i++)
	{
		std::vector<std::string> secondSplit;
		
		ret = comn::StringUtil::split(firstSplit[i],':',secondSplit);
		if(ret == 2)
		{
			JPlayer_RecordItem item;
			//item.m_beginTime = my_atoi64(secondSplit[0].c_str());
			//item.m_endTime   = my_atoi64(secondSplit[1].c_str());
			item.m_type = 0;

			if (resp.m_sum >= countRsp)
			{
				break;
			}

			resp.m_pItems[resp.m_sum++] = item;
		}
	}

	return 0;
}



LONG JPlayer::queryRecord(const JPlayer_QueryRecordRequest& req, 
				 JPlayer_QueryRecordResponse& resp)
{
	

	return JPLAYER_MSG_NORMAL;
}

int JPlayer::requestVodStream(const JPlayer_RecordItem &item)
{
	
	return JPLAYER_MSG_NORMAL;

}



LONG  JPlayer::setPlayPos(LLONG lRelativePos)
{
	return JPLAYER_MSG_NORMAL;
}

LLONG JPlayer::getPlayPos()
{
	if(m_pSubscriberClient == NULL)
		return 0;
//	return m_pSubscriberClient->getPlayPos();
//	return getAvSyncManager().getLastVideoPts();
	return (getAvSyncManager().getLastVideoPts() - m_pSubscriberClient->getPlayPos()*1000);
}

LONG  JPlayer::pause(unsigned int nPause)
{

	 if (nPause == 1)
	 {
		 getVideoManager().pause(true);
		 getAudioManager().pause(true);
	 }
	 else
	 {
		 getVideoManager().pause(false);
		 getAudioManager().pause(false);
	 }

	return JPLAYER_MSG_NORMAL;
}

LONG  JPlayer::setScale(float scale)
{
	getVideoManager().setScale(scale);

    return JPLAYER_MSG_NORMAL;
}

float JPlayer::getScale()
{
	return JPLAYER_MSG_NORMAL;
}

LONG JPlayer::playSound()
{
	return getAudioManager().playSound();
}

LONG JPlayer::stopSound()
{
	return getAudioManager().stopSound();
}

LONG JPlayer::setVolume(unsigned int  volume)
{
	return getAudioManager().setVolume(volume);
}

LONG JPlayer::getVolume(unsigned int*  pVolume)
{
	return getAudioManager().getVolume(pVolume);
}



///����ý�����
int JPlayer::writeMediaPacket(const StreamPacket& packet,
							  bool isLive) 
{
	int ret = 0;
	
	if (packet.flag == JPLAYER_FRAME_H264)
	{
		ret = getVideoManager().writeMediaPacket(packet);
	}
	else if (packet.flag == JPLAYER_FRAME_AUDIO)
	{
		ret = getAudioManager().writeMediaPacket(packet);
	}

	m_bd.setFrame(packet.flag,packet.size,packet.timestamp);
	
	m_recorder.writeMediaPacket(packet);
	
	return ret;
}


void JPlayer::notifyMediaStream(LONG errorType,LLONG para1,LLONG para2)
{
	if (m_jpMsgCallback != NULL)
	{
		(*m_jpMsgCallback)((LONG)this,errorType,para1,para2,m_msgDwUser);
	}

	if (errorType == JPLAYER_MSG_VIDEO_RESOLUTION)
	{
		getVideoManager().flushDecoder();
		getAudioManager().clearAllBuffer();

		if (m_videoWidth != para1) 
		{
			stopRecord();

			m_videoWidth = para1;
		}
	}
}



LONG JPlayer::snap(const char *path)
{
	return getVideoManager().snap(path)?JPLAYER_MSG_NORMAL:JPLAYER_MSG_NO_INIT_FAIL;
}

LONG JPlayer::sendStream(
						 LLONG          serialNum, 
						 unsigned int   frameType, 
						 int            isKey, 
						 unsigned char* pFrame, 
						 unsigned int   frameLength, 
						 LLONG          timestamp)
{


	if ( m_pSubscriberClient == NULL )
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	comn::AutoCritSec lock(m_csTalk);

	// special flag
	if(frameLength == 0)
	{
		// end of stream
		m_pSubscriberClient->SendStream(0,
					frameType,
					isKey,
					NULL,
					0,
					timestamp);
		return JPLAYER_MSG_NORMAL;
	}

	StreamPacket packet;
	packet.data = pFrame;
	packet.size = frameLength;


	do 
	{
		bool ok = getAudioManager().encodeAudio(packet);
		JPLAYER_LOG_ERROR("JPlayer::sendStream ok: %d\n",ok);
		if (!ok)
		{
			return JPLAYER_MSG_NORMAL;
		}

		static LLONG  serialNumReal = 0;
		serialNumReal ++;
		m_pSubscriberClient->SendStream(serialNumReal,
			frameType,
			isKey,
			packet.data,
			packet.size,
			timestamp);

		packet.size = 0;
	} while (1);


	return JPLAYER_MSG_NORMAL;

}



LONG JPlayer::setBDCallBack(unsigned int nSecond,
				   JPlayerBDCallBack jpBDCallback,
				   LONG dwUser)
{
	
	return m_bd.setBDCallBack((LONG)this,nSecond,jpBDCallback,dwUser);

}



LONG JPlayer::sendMessage(const char *pMsg)
{
	
	return JPLAYER_MSG_NO_INIT_FAIL;
}



LONG  JPlayer::startRecord(int track,const char *path)
{
	bool ok = m_recorder.openFile(track,path);
	if (!ok)
	{
		return JPLAYER_MSG_RECORD_ERROR;
	}
	
	return JPLAYER_MSG_NORMAL;	
}


LONG  JPlayer::stopRecord()
{
	LONG msg = JPLAYER_MSG_RECORD_INVALID_FILE;

	bool ok = m_recorder.closeFile();
	if (ok)
	{
		msg = JPLAYER_MSG_NORMAL;
	}

	return msg;
}

LONG JPlayer::locateStream(long p_CurrentTime)
{
	if(m_pSubscriberClient == NULL)
		return -1;
	long nRet = m_pSubscriberClient->locateStream(p_CurrentTime);
	getAudioManager().setCurrentTime(p_CurrentTime);
	getVideoManager().setCurrentTime(p_CurrentTime);
	return nRet;
}

//int JPlayer::openAudioCapture(const char *p_StreamPath)
//{
//	if(m_pSubscriberClient == NULL)
//		return 0;
//	return m_pSubscriberClient->openAudioCapture(p_StreamPath);
//}

int JPlayer::setRecordPlaySource(long from,long to)
{
	if(m_pSubscriberClient == NULL)
	{
		m_pSubscriberClient = new(std::nothrow) ISCMSubscriberClient(m_sess,this);
		p_ISCMSubscriberClient = m_pSubscriberClient;
	}
	return m_pSubscriberClient->setRecordPlaySource(from,to);
}

bool  JPlayer::clearAllBuffer()
{
	getAudioManager().clearAllBuffer();
	return getVideoManager().clearAllBuffer();
}

void JPlayer::playAudio(char * data, int datalen)
{
	m_pSubscriberClient->playAudio(data,datalen);
}

void JPlayer::seekSuccess()
{
	getAudioManager().seekSuccess();
	getVideoManager().seekSuccess();
}
