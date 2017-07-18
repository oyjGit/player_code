#ifdef WIN32
#include "npdebug.h"
#include <Windows.h>
#endif
#include "PlayerManager.h"
#include <algorithm>
#include "H264Decode.h"
#include "Logs.h"
#include "GlobalFunc.h"


#ifdef XBMC_DECODE
#include "XbmcDecode.h"
#endif

bool PlayerManager::ms_hasInit = false;

/////////////////////////////////////////////////////////////////////////////
PlayerManager::PlayerManager()
{
	m_jpMsgCallback = NULL;
}

PlayerManager::~PlayerManager()
{
	
}

bool PlayerManager::init()
{
	if (!ms_hasInit)
	{
		H264Decode::startup();

#ifdef XBMC_DECODE
		XbmcDecode::startup();
#endif

		ms_hasInit = true;
	}

	return true;
}


bool PlayerManager::unInit()
{
	if (ms_hasInit)
	{
		H264Decode::cleanup();

#ifdef XBMC_DECODE
		XbmcDecode::cleanup();
#endif

		ms_hasInit = false;
	}


	return true;
}

void PlayerManager::closeAllPlayer()
{
	bool hasPlayer = false;

	{
		JPlayerLst::iterator it = m_playerLst.begin();

		for (; it != m_playerLst.end(); it ++)
		{
#ifdef NOT_USE_SHAREDPTR
			delete (*it);
#endif

		}

		if (m_playerLst.size() > 0)
		{
			hasPlayer = true;
		}

		m_playerLst.clear();
	}

	if (hasPlayer)
	{
		JPLAYER_LOG_INFO("closeAllPlayer\n");
	}
}



JPlayerPtr PlayerManager::findPlayer()
{
	if (!m_playerLst.empty())
	{
		return m_playerLst[0];
	}

	JPLAYER_LOG_ERROR("has not findPlayer\n");

#ifndef NOT_USE_SHAREDPTR
	return JPlayerPtr();
#else
	return NULL;
#endif

}


JPlayerPtr PlayerManager::findPlayer(LONG lHandle)
{
	JPlayerLst::iterator it = std::find_if(m_playerLst.begin(),
							m_playerLst.end(),
							std::bind2nd(JPlayerFinder(),lHandle) );
	if (it != m_playerLst.end())
	{
		return (*it);
	}


	JPLAYER_LOG_ERROR("has not findPlayer:%ld\n",lHandle);


#ifndef NOT_USE_SHAREDPTR
	return JPlayerPtr();
#else
	return NULL;
#endif
	
}


LONG PlayerManager::JPlayer_GetLastError(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_GetLastError\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	return pPlayer->getLastError();
}

LONG PlayerManager::JPlayer_SetMsgCallBack(
										   JPlayerMsgCallBack jpMsgCallback,	
										   LONG dwUser)
{
	m_jpMsgCallback = jpMsgCallback;
	m_msgDwUser = dwUser;

	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetMsgCallBack\n");

	JPlayerLst::iterator it = m_playerLst.begin();
    for (; it != m_playerLst.end(); it++)
    {
		(*it)->setMsgCallBack(m_jpMsgCallback,m_msgDwUser);
    }
	return JPLAYER_MSG_NORMAL;
}



LONG PlayerManager::JPlayer_Open(const JPlayer_PlaySession &sess)
{
	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

	closeAllPlayer();

	JPLAYER_LOG_INFO("JPlayer_Open\n");

	JPlayer *pPlayer = new(std::nothrow) JPlayer(sess);
	if (pPlayer == NULL)
	{
		return 0;
	}

	if (m_jpMsgCallback != NULL)
	{
		pPlayer->setMsgCallBack(m_jpMsgCallback,m_msgDwUser);
	}
	

	LONG ret = pPlayer->openAgent();
	if ( ret == JPLAYER_MSG_NORMAL )
	{
		JPlayerPtr sPtr(pPlayer);

		m_playerLst.push_back(sPtr);

		return (LONG)pPlayer;
	}
	else
	{
		delete pPlayer;
	}
	
	return 0;
}

LONG PlayerManager::JPlayer_Close(LONG lHandle)
{
	(void)lHandle;

	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

    JPLAYER_LOG_INFO("JPlayer_Close\n");

	closeAllPlayer();

	return JPLAYER_MSG_NORMAL;


/*
	comn::AutoRWLock wlock(m_csPlayer,false);

	JPlayerLst::iterator it = std::find_if(m_playerLst.begin(),
		m_playerLst.end(),
		std::bind2nd(JPlayerFinder(),lHandle) );
	if (it != m_playerLst.end())
	{
		//(*it)->closeAgent();

#ifndef NOT_USE_SHAREDPTR
		
#else
		delete (*it);
#endif
		m_playerLst.erase(it);

		return JPLAYER_MSG_NORMAL;
	}

	return JPLAYER_MSG_NO_INIT_FAIL;
	*/
}

LONG PlayerManager::JPlayer_SetPlaySession(LONG lHandle,
										  const JPlayer_PlaySession &sess)
{
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_Play_Live\n");

	JPlayerPtr pPlayer(findPlayer());
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->setPlaySession(sess);
}


LONG PlayerManager::JPlayer_Play_Live(LONG lHandle ,int p_StreamMode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength)
{
	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_Play_Live\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->playAgent(p_StreamMode, p_MinMediaBufferLength, p_MaxMediaBufferLength);
}

LONG PlayerManager::JPlayer_Play_Live(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_Play_Live\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->playAgent();
}


LONG PlayerManager::JPlayer_QueryRecord(LONG lHandle,
						 const JPlayer_QueryRecordRequest& req, 
						 JPlayer_QueryRecordResponse& resp)
{
	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_QueryRecord\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->queryRecord(req,resp);
}

LONG PlayerManager::JPlayer_Play_Record(LONG lHandle,const JPlayer_RecordItem &item)
{
	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_Play_Record\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->playAgent(item);
}


LONG PlayerManager::JPlayer_Stop(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_Stop\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	JPLAYER_LOG_INFO("JPlayer_stopAgent\n");
	return pPlayer->stopAgent();
}


LONG  PlayerManager::JPlayer_SetPlayPos(LONG lHandle, LLONG lRelativePos)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetPlayPos\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->setPlayPos(lRelativePos);
}

LLONG PlayerManager::JPlayer_GetPlayPos(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_GetPlayPos\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->getPlayPos();
}

LONG PlayerManager::JPlayer_Pause(LONG lHandle,unsigned int nPause)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_Pause\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->pause(nPause);
}

LONG  PlayerManager::JPlayer_SetScale(LONG lHandle,float scale)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetScale\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->setScale(scale);
}

float PlayerManager::JPlayer_GetScale(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_GetScale\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->getScale();
}

LONG PlayerManager::JPlayer_PlaySound(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_PlaySound\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->playSound();
}

LONG PlayerManager::JPlayer_StopSound(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_StopSound\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->stopSound();
}



LONG  PlayerManager::JPlayer_SetVolume(unsigned int  volume)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetVolume\n");

	JPlayerPtr pPlayer(findPlayer());
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->setVolume(volume);
}


LONG  PlayerManager::JPlayer_GetVolume(unsigned int*  pVolume)
{
	if (pVolume == NULL)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_GetVolume\n");

	JPlayerPtr pPlayer(findPlayer());
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->getVolume(pVolume);
}

LONG  PlayerManager::JPlayer_Snap(LONG lHandle,const char *path)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	LONG ret = pPlayer->snap(path);

	JPLAYER_LOG_INFO("JPlayer_Snap\n");

	return ret;
}

LONG  PlayerManager::JPlayer_SendStream(LONG lHandle,
										LLONG          serialNum, 
										unsigned int   frameType, 
										int            isKey, 
										unsigned char* pFrame, 
										unsigned int   frameLength, 
										LLONG          timestamp)
{

	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	
	return pPlayer->sendStream(serialNum, 
		frameType, 
		isKey, 
		pFrame, 
		frameLength, 
		timestamp);
}


LONG PlayerManager::JPlayer_SetBDCallBack(
						   LONG lHandle,
						   unsigned int nSecond,
						   JPlayerBDCallBack jpBDCallback,
						   LONG dwUser)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetBDCallBack\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->setBDCallBack(
		nSecond,
		jpBDCallback, 
		dwUser);
}

LONG  PlayerManager::JPlayer_SetAudioEncode(LONG lHandle,
							int sampleRate,
							int channel,
							int fps)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetAudioEncode\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return getAudioManager().setAudioEncodePara(sampleRate,
		channel,
		AUDIO_BIT_RATES,
		fps);

}




LONG PlayerManager::JPlayer_SetAudioDecode(LONG lHandle,
							int sampleRate,
							int channel)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetAudioDecode\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	return getAudioManager().setAudioDecodePara(sampleRate,
		channel,
		AUDIO_BIT_RATES);

}

LONG  PlayerManager::JPlayer_SendMessage(LONG lHandle,
						  const char *pMsg)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SendMessage\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	return pPlayer->sendMessage(pMsg);

}

LONG  PlayerManager::JPlayer_StartRecord(LONG lHandle,int track,const char *path)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_StartRecord\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	return pPlayer->startRecord(track,path);
}

LONG  PlayerManager::JPlayer_StopRecord(LONG lHandle)
{
	///��������ӿ�
	comn::AutoCritSec rlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_StopRecord\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	return pPlayer->stopRecord();
}


//LONG  PlayerManager::JPlayer_SetStreamMode(LONG lHandle,int picMode)
//{
//	///��������ӿ�
//	comn::AutoCritSec rlock(m_csPlayer);
//
//	JPLAYER_LOG_INFO("JPlayer_SetStreamMode\n");
//
//	JPlayerPtr pPlayer(findPlayer(lHandle));
//	if (!pPlayer)
//	{
//		return JPLAYER_MSG_NO_INIT_FAIL;
//	}
//	return getVideoManager().setStreamMode(picMode);
//}

LONG PlayerManager::JPlayer_LocateStream(LONG lHandle, long p_CurrentTime)
{
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_LocateStream\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->locateStream(p_CurrentTime);
}

//LONG PlayerManager::JPlayer_OpenAudioCapture(LONG lHandle, const char *p_StreamPath)
//{
//	comn::AutoCritSec wlock(m_csPlayer);
//
//	JPLAYER_LOG_INFO("JPlayer_LocateStream\n");
//
//	JPlayerPtr pPlayer(findPlayer(lHandle));
//	if (!pPlayer)
//	{
//		return JPLAYER_MSG_NO_INIT_FAIL;
//	}
//	return pPlayer->openAudioCapture(p_StreamPath);
//}

LONG PlayerManager::JPlayer_SetRecordPlaySource(LONG lHandle,long from,long to)
{
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("JPlayer_SetRecordPlaySource\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return pPlayer->setRecordPlaySource(from,to);
}

void PlayerManager::playAudio(LONG lHandle, char * data, int datalen)
{
	comn::AutoCritSec wlock(m_csPlayer);

	JPLAYER_LOG_INFO("playAudio\n");

	JPlayerPtr pPlayer(findPlayer(lHandle));
	if (!pPlayer)
	{
		return /*JPLAYER_MSG_NO_INIT_FAIL*/;
	}
	/*return*/ pPlayer->playAudio(data,datalen);
}




