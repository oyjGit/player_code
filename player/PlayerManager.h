#ifndef PLAYER_MANAGER_H_
#define PLAYER_MANAGER_H_

#include "JPlayer.h"
#include <vector>
#include "TCriticalSection.h"
#include <functional>

/////////////////////////////////////////////////////////////////////////////


#ifndef NOT_USE_SHAREDPTR

struct JPlayerFinder : public std::binary_function<JPlayerPtr,LONG,bool>
{
	bool operator()(const JPlayerPtr &lst,LONG ptr) const
	{
		return lst.get() == (JPlayer *)ptr;		
	}
};

#else

struct JPlayerFinder : public std::binary_function<JPlayerPtr,LONG,bool>
{
	bool operator()(const JPlayerPtr &lst,LONG ptr) const
	{
		return lst == (JPlayer *)ptr;		
	}
};

#endif





class PlayerManager
{
public:
    PlayerManager();
    virtual ~PlayerManager();

    static bool init();
	static bool unInit();

	LONG JPlayer_GetLastError(LONG lHandle);

	LONG JPlayer_SetMsgCallBack(
		JPlayerMsgCallBack jpMsgCallback,LONG dwUser);


	LONG JPlayer_Open(const JPlayer_PlaySession &sess);
	LONG JPlayer_Close(LONG lHandle);
	LONG JPlayer_SetPlaySession(LONG lHandle,const JPlayer_PlaySession &sess);

	LONG JPlayer_Play_Live(LONG lHandle);
	LONG JPlayer_Play_Live(LONG lHandle ,int p_StreamMode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength);
	LONG JPlayer_QueryRecord(LONG lHandle,
		const JPlayer_QueryRecordRequest& req, 
		JPlayer_QueryRecordResponse& resp);
	LONG JPlayer_Play_Record(LONG lHandle,const JPlayer_RecordItem &item);
	LONG JPlayer_Stop(LONG lHandle);

	LONG JPlayer_PlaySound(LONG lHandle);
	LONG JPlayer_StopSound(LONG lHandle);

	LONG  JPlayer_SetPlayPos(LONG lHandle, LLONG lRelativePos);
	LLONG JPlayer_GetPlayPos(LONG lHandle);
	
	LONG  JPlayer_Pause(LONG lHandle,unsigned int nPause);

	LONG  JPlayer_SetScale(LONG lHandle,float scale);
	float JPlayer_GetScale(LONG lHandle);

	
	LONG  JPlayer_SetVolume(unsigned int  volume);
	LONG  JPlayer_GetVolume(unsigned int*  pVolume);

	LONG  JPlayer_Snap(LONG lHandle,const char *path);

	LONG  JPlayer_SendStream(LONG lHandle,
		LLONG          serialNum, 
		unsigned int   frameType, 
		int            isKey, 
		unsigned char* pFrame, 
		unsigned int   frameLength, 
		LLONG          timestamp);

	LONG JPlayer_SetBDCallBack(
		LONG lHandle,
		unsigned int nSecond,
		JPlayerBDCallBack jpBDCallback,
		LONG dwUser);


	LONG  JPlayer_SetAudioEncode(LONG lHandle,
		int sampleRate,
		int channel,
		int fps);
	

	LONG JPlayer_SetAudioDecode(LONG lHandle,
		int sampleRate,
		int channel);

	LONG  JPlayer_SendMessage(LONG lHandle,
		const char *pMsg);


	LONG  JPlayer_StartRecord(LONG lHandle,int track,const char *path);

	LONG  JPlayer_StopRecord(LONG lHandle);

//	LONG  JPlayer_SetStreamMode(LONG lHandle,int picMode);

	LONG JPlayer_LocateStream(LONG lHandle,long p_CurrentTime);

//	LONG JPlayer_OpenAudioCapture(LONG lHandle, const char *p_StreamPath);

	LONG JPlayer_SetRecordPlaySource(LONG lHandle,long from,long to);

	void playAudio(LONG lHandle, char * data, int datalen);

protected:
	typedef std::vector<JPlayerPtr> JPlayerLst;

    JPlayerPtr  findPlayer(LONG lHandle);
	JPlayerPtr  findPlayer();
	void closeAllPlayer();

private:
	JPlayerLst m_playerLst;
    comn::CriticalSection m_csPlayer;

	JPlayerMsgCallBack m_jpMsgCallback;
	LONG m_msgDwUser;

	static bool ms_hasInit;

};


/////////////////////////////////////////////////////////////////////////////

#endif /* PLAYER_MANAGER_H_ */



