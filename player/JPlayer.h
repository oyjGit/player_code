#ifndef JPLAYER_H
#define JPLAYER_H

#include "NetPosaJPlayerSDK.h"
#include <string>

//#define NOT_USE_SHAREDPTR

#ifndef NOT_USE_SHAREDPTR
#include "SharedPtr.h"
#endif

#include "MediaInfo.h"
#include "TCriticalSection.h"
#include "BandWidthManager.h"
#include "Mp4RecordWrap.h"

//#include "common.h"

class ISCMSubscriberClient;


class JPlayer
{
public:
    explicit JPlayer(const JPlayer_PlaySession &sess);
    virtual ~JPlayer();

	JPlayer_PlaySession &getSessionInfo() 
	{
		return m_sess;
	}

	LONG setPlaySession(const JPlayer_PlaySession &sess);

	LONG openAgent();
	LONG closeAgent();

	LONG playAgent( int p_StreamMode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength);
	LONG playAgent();
	LONG playAgent(const JPlayer_RecordItem &item);
	LONG stopAgent();

	LONG getLastError();

	LONG setMsgCallBack(JPlayerMsgCallBack jpMsgCallback,LONG dwUser);


	LONG queryRecord(const JPlayer_QueryRecordRequest& req, 
		JPlayer_QueryRecordResponse& resp);

	LONG  setPlayPos(LLONG lRelativePos);
	LLONG getPlayPos();

	LONG  pause(unsigned int nPause);

	LONG  setScale(float scale);
	float getScale();

	LONG playSound();
	LONG stopSound();

	LONG setVolume(unsigned int  volume);
    LONG getVolume(unsigned int*  pVolume);

	LONG snap(const char *path);
	LONG sendStream( LLONG          serialNum, 
		unsigned int   frameType, 
		int            isKey, 
		unsigned char* pFrame, 
		unsigned int   frameLength, 
		LLONG          timestamp);

	LONG setBDCallBack(unsigned int nSecond,
		JPlayerBDCallBack jpBDCallback,
		LONG dwUser);


	LONG sendMessage(const char *pMsg);

	///����ý�����
	int writeMediaPacket(const StreamPacket& packet,bool isLive = true) ;

	void notifyMediaStream(LONG errorType,LLONG para1,LLONG para2);

	int requestVodStream(const JPlayer_RecordItem &item);

	LONG  startRecord(int track,const char *path);
	LONG  stopRecord();

	//void saveLastIFrame(const StreamPacket& packet);

	LONG locateStream(long p_CurrentTime);

//	int openAudioCapture(const char *p_StreamPath);

	int setRecordPlaySource(long from,long to);

	bool  clearAllBuffer();

	void playAudio(char * data, int datalen);

	void seekSuccess();

private:
	ISCMSubscriberClient *m_pSubscriberClient;
	JPlayer_PlaySession m_sess;

	JPlayerMsgCallBack m_jpMsgCallback;
	LONG m_msgDwUser;


	CBandWidth m_bd;

	comn::CriticalSection m_csTalk;

	MP4RecorderWrap m_recorder;

	int m_videoWidth;

};

#ifndef NOT_USE_SHAREDPTR
typedef std::shared_ptr<JPlayer> JPlayerPtr;
#else
typedef JPlayer* JPlayerPtr;
#endif

#endif




