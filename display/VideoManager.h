#ifndef VIDEO_MANAGER_H_
#define VIDEO_MANAGER_H_

#include "NetPosaJPlayerSDK.h"
#include "MediaSink.h"
#include "TYuvMemQueue.h"
#include "H264Decode.h"
#include "TThread.h"
#include "TEvent.h"
#include "TMemQueue.h"
#include "PlayerConfig.h"
#include "TTime.h"


#ifdef XBMC_DECODE
class XbmcDecode;
#endif

#ifdef OMX_DECODE
class JOmxDecode;
#endif



class VideoManager : public TMediaSink,public comn::CMemQueue,public comn::Thread
{
public:
	VideoManager();
	virtual ~VideoManager();

	virtual bool open() ;
	virtual bool writeFrame(const JPlayer_MediaFrame& frame);
	virtual void close() ;

	///����ý�����
	virtual int writeMediaPacket(const StreamPacket& packet) ;

	virtual int run();


	LONG setMsgCallBack(JPlayerMsgCallBack jpMsgCallback,LONG dwUser,LONG lHandle);

	virtual bool snap(const char *path);

	void  doCallbackMsg(LONG msg,uint64_t timestamp);

	LONG  setStreamMode(int picMode);

	//����
#ifdef ANDROID_NDK
	int   simpleDrawFrame();
#endif
	bool  pause(bool hasPause);
	bool  setScale(float scale);
	bool  flushDecoder();

	int   getYUV420PFrame(int milltimes,JPlayer_MediaFrame& frame);
	void  releaseYUV420PFrame(JPlayer_MediaFrame& frame);
	void  signalYUVQueue();

	bool setSurface(void *surface);

	bool  clearAllBuffer();

	void  setCurrentTime(long p_CurrentTime);

	void setBeginTime(long p_BeginTime);

	void setVideoStreamMode(int p_Mode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength);

	void seekSuccess();
private:

	int getFrameType(unsigned char *framebuffer,int framesize, int *frametype);
	unsigned int Sys_GetTickCount1(void);

	void  doHasPicCallback(uint64_t timestamp,const int width = 0, const int height = 0);

	void handleJitter(int tsElapseTimer,int timeinterval);

	int str_sunday_search(char *sur,int p_surlen, char *tar, int p_tarlen);

private:
	bool m_hasInit;
	bool m_hasPause;
	bool m_hasExit;
	int  m_clearDisplauNum;

	H264Decode m_h264Decoder;
	comn::CYuvMemQueue m_yuvQueue;
	comn::Event m_decodeWaiter;
	comn::Event m_packetWaiter;
	JPlayer_MediaFrame m_lastFrame;

	JPlayerMsgCallBack m_jpMsgCallback;
	LONG m_msgDwUser;
	LONG m_playerHandle;

#ifdef XBMC_DECODE
	XbmcDecode *m_pXbmcDecoder;
#endif

#ifdef OMX_DECODE
	JOmxDecode *m_pOmxDecoder;
#endif

#ifdef USE_HARD_DECODER
	bool m_doSnap;
#endif

	float m_scale;

	int m_picCount;

	unsigned long long startPts;

	unsigned int startTime;

	int videoStreamMode;

	int playFrameCount;

	unsigned long long lastPts;

	bool m_FirstDraw;

	comn::Time m_tsElapse;
	comn::Time m_FrameIntervalElapse;

	int m_FirstOpenFlag;

	unsigned long long m_lastpushtime;

	unsigned int m_lastAdjusttime;

	unsigned int m_nFirstDelayTime;
	int m_nTimeDelayCount;
	unsigned int m_nTimeDelayList[3] = {0};
	unsigned int m_nTimeDelayValueList[3] = {0};

	unsigned long long m_DropStreamTime;

	int m_MinFullBufferTime;

	int m_ChangeFirstTime;
	unsigned int m_ChangeBeginTime;
	int m_ChangeFlag;

	int aDecodeTimelist[20] = {0};
	int aDeocdeCount = 0;
	int m_MinBufferTime = 0;
	int m_MaxBufferTime = 0;
	bool m_BufferEmpty;

	bool m_SeekSuccess;
};



/////////////////////////////////////////////////////////////////////////////

#endif /* VIDEO_MANAGER_H_ */



