#ifdef OS_MAC
#include <sys/time.h>
#endif
#include "Logs.h"
#include "GlobalFunc.h"
#include "IosDisplayInterface.h"
#include "TTime.h"

 #include <unistd.h>

#ifdef XBMC_DECODE
#include "XbmcDecode.h"
#endif

#ifdef OMX_DECODE
#include "OmxDecode.h"
#endif

#define DEFAULT_VIDEO_FPS (15)
#define DEFAULT_VIDEO_FRAME_CALLBACK (3)

enum VideoStreamMode
{
	UDP_VIDEO_STREAM_MODE = 1,
	RECORD_VIDEO_STREAM_MODE = 2,
	RTMP_VIDEO_STREAM_MODE = 3,
};

VideoManager::VideoManager()
	:m_hasInit(false),
	 m_hasPause(false),
	 m_hasExit(false),
	 m_clearDisplauNum(0),
	 m_jpMsgCallback(NULL),
#ifdef XBMC_DECODE
	m_pXbmcDecoder(NULL),
#endif
#ifdef OMX_DECODE
	 m_pOmxDecoder(NULL),
#endif
#ifdef USE_HARD_DECODER
	 m_doSnap(false),
#endif
m_scale(1.0),
m_picCount(0),
startPts(0),
startTime(0),
lastPts(0),
m_FirstDraw(true),
videoStreamMode(UDP_VIDEO_STREAM_MODE),
playFrameCount(0),
m_FirstOpenFlag(1),
m_lastpushtime(0),
m_lastAdjusttime(0),
m_nTimeDelayCount(0),
m_nFirstDelayTime(0),
m_MinFullBufferTime(0),
m_ChangeFirstTime(0),
m_ChangeBeginTime(0),
m_ChangeFlag(0),
m_MaxBufferTime(0),
m_MinBufferTime(0),
m_BufferEmpty(false),
m_SeekSuccess(false)
{
	m_lastFrame.m_pData[0] = NULL;
}

VideoManager::~VideoManager()
{
	stop();

	if (m_h264Decoder.isOpen())
	{
		m_h264Decoder.close();
		m_h264Decoder.setMediaSink(NULL);

		JPLAYER_LOG_INFO("h264Decoder closed\n");
	}

	if (m_hasInit)
	{
		unInit();
		m_yuvQueue.unInit();
		m_hasInit = false;
	}

#ifdef XBMC_DECODE
	if (m_pXbmcDecoder != NULL)
	{
		delete m_pXbmcDecoder;
		m_pXbmcDecoder = NULL;
	}
#endif

#ifdef OMX_DECODE
	if (m_pOmxDecoder != NULL)
	{
		delete m_pOmxDecoder;
		m_pOmxDecoder = NULL;
	}
#endif

}



LONG  VideoManager::setStreamMode(int picMode)
{
	return JPLAYER_MSG_NORMAL;
}

bool VideoManager::open()
{
	if (!m_hasInit)
	{
		bool ok = true;

#ifndef USE_HARD_DECODER
		ok = m_yuvQueue.init(YUV_MAX_FRAME_POOL * MB);
		if (!ok)
		{
			m_yuvQueue.unInit();
			JPLAYER_LOG_ERROR("VideoManager init yuv queue fail\n");
			return false;
		}
#endif

		if (!init(VIDEO_MAX_PACKET_POOL * MB))
		{
			unInit();
			m_yuvQueue.unInit();
			JPLAYER_LOG_ERROR("VideoManager init packet queue fail\n");
			return false;
		}

		ok = m_h264Decoder.open();
		if (!ok)
		{
			unInit();
			m_yuvQueue.unInit();
			JPLAYER_LOG_ERROR("h264Decoder open fail\n");
			return false;
		}
		else
		{
			JPLAYER_LOG_INFO("h264Decoder open ok\n");
		}

#ifndef USE_HARD_DECODER
		m_h264Decoder.setMediaSink(this);
#endif

		m_hasInit = true;

		start();
	}

	m_picCount = 0;
	m_hasExit = false;
	m_FirstDraw = true;
	JPLAYER_LOG_INFO("simpleDrawFrame hasExit clearDisplay open");
	m_clearDisplauNum = 0;
	m_scale = 1.0;
	JPLAYER_LOG_INFO("VideoManager init ok\n");

	playFrameCount = 0;

	memset(m_nTimeDelayList,0,12);
	memset(m_nTimeDelayValueList,0,12);
	m_nTimeDelayCount = 0;
	m_MinFullBufferTime = 0;
	m_ChangeFirstTime = 0;
	m_ChangeBeginTime = 0;
	m_ChangeFlag = 0;

	memset(aDecodeTimelist,0,80);
	aDeocdeCount = 0;
//	m_tsElapse.reset();
//	m_FirstOpenFlag = 1;

	return true;
}



bool VideoManager::writeFrame(const JPlayer_MediaFrame& frame)
{
	if (m_hasExit)
	{
		return false;
	}

	int count = 6;

	bool ok = false;
#ifndef USE_HARD_DECODER
	///���Է������
	do
	{
//		JPLAYER_LOG_INFO(" writeFrame  : %llu\n",frame.m_timestamp);
//		JPLAYER_LOG_INFO(" m_yuvQueue  1: %d\n",m_yuvQueue.size());
		ok = m_yuvQueue.push(frame);
//		JPLAYER_LOG_INFO(" m_yuvQueue  2: %d\n",m_yuvQueue.size());
		if (ok)
		{
			break;
		}

		///�Լ��ȴ�1����
		m_decodeWaiter.timedwait(15);

	} while (count--);


	///���Զ��Ҳ�Ų����룬��������֡�ˣ���֮ǰ��֡
	if (!ok)
	{
		ok = m_yuvQueue.push(frame,true);
	}

	if (ok)
	{
//		doHasPicCallback(frame.m_timestamp);

		doHasPicCallback(frame.m_timestamp,frame.m_width,frame.m_height);
	}
#endif
	return ok;
}

void  VideoManager::doCallbackMsg(LONG msg,uint64_t timestamp)
{
	if (m_jpMsgCallback != NULL)
	{
		(*m_jpMsgCallback)(m_playerHandle,msg,timestamp,0,m_msgDwUser);
	}
}

void  VideoManager::doHasPicCallback(uint64_t timestamp,const int width, const int height)
{
	// dont do this, will caused UI loading too fast if speed is very slow.
	//if (m_picCount == 0)
	{
		int tsElapseTimer = 0;
		if(m_FirstOpenFlag == 1)
		{
			m_FirstOpenFlag=0;
			tsElapseTimer = m_tsElapse.getElapse();
		}
//		JPLAYER_LOG_INFO("tsElapseTimer open time :%d",tsElapseTimer);
		doCallbackMsg(JPLAYER_MSG_FIRSTFRAME_TIME,tsElapseTimer);


		if (m_jpMsgCallback != NULL)
		{
			(*m_jpMsgCallback)(m_playerHandle,JPLAYER_MSG_DECODE_RESOLUTION,width,height,m_msgDwUser);
		}

//		doCallbackMsg(JPLAYER_MSG_VIDEO_IS_READY,timestamp);
	}

	if (m_picCount++ >= DEFAULT_VIDEO_FRAME_CALLBACK)
	{
		m_picCount = 0;
	}
}


LONG VideoManager::setMsgCallBack(JPlayerMsgCallBack jpMsgCallback,
                                  LONG dwUser,
                                  LONG lHandle)
{
	m_jpMsgCallback = jpMsgCallback;
	m_msgDwUser = dwUser;
	m_playerHandle = lHandle;

	return JPLAYER_MSG_NORMAL;
}

bool  VideoManager::flushDecoder()
{
	clearAllBuffer();
	m_h264Decoder.flushBbuffers();
	return true;
}


void  VideoManager::signalYUVQueue()
{
	m_yuvQueue.signalQueue();
}

void VideoManager::close()
{
	JPLAYER_LOG_INFO("simpleDrawFrame hasExit clearDisplay close");
	m_hasExit = true;
	m_hasPause = false;

	m_clearDisplauNum = 2;
	m_scale = 1.0;
	m_picCount = 0;

	flushDecoder();

#ifndef USE_HARD_DECODER
	m_yuvQueue.signalQueue();

#ifdef ANDROID_NDK
	m_decodeWaiter.timedwait(1000);
#endif

#else

#ifdef XBMC_DECODE
	if (m_pXbmcDecoder != NULL)
	{
		return m_pXbmcDecoder->close();
	}
#endif

#ifdef OMX_DECODE
	if (m_pOmxDecoder != NULL)
	{
		return m_pOmxDecoder->close();
	}
#endif
#endif
	JPLAYER_LOG_INFO("VideoManager uninit ok\n");
}

bool VideoManager::snap(const char *path)
{
#ifdef USE_HARD_DECODER
	bool ok = false;

	m_doSnap = true;

	ok = m_h264Decoder.snapWait(path);

	m_h264Decoder.flushBbuffers();

	m_doSnap = false;

	return ok;
#else
	return m_h264Decoder.snap(path);
#endif

}

///����ý�����
int VideoManager::writeMediaPacket(const StreamPacket& packet)
{
	if (m_hasExit)
	{
		return 0;
	}

	if(packet.timestamp < m_lastpushtime && m_lastpushtime > 0xff0000 && (videoStreamMode == UDP_VIDEO_STREAM_MODE || videoStreamMode == RTMP_VIDEO_STREAM_MODE))
	{
		JPLAYER_LOG_INFO("time Rollback video push data \n");
		m_lastpushtime = packet.timestamp + 0xfffffe;
	}
	else
		m_lastpushtime = packet.timestamp;

	int count = 5;

//	if(m_ChangeBeginTime == 0)
//		m_ChangeBeginTime = Sys_GetTickCount1();
//
//	if(isPlayIFrame > 5)
//	{
//		int deltatime = (Sys_GetTickCount1() - startTime) - (packet.timestamp-startPts);
//		if(deltatime<0)
//		{
//			startTime = Sys_GetTickCount1() - (packet.timestamp-startPts);
//
//			m_ChangeFirstTime += -deltatime;
//
//			JPLAYER_LOG_INFO("change start time :%d  %d\n",-deltatime,m_ChangeFirstTime);
//		}
//
//	}

	///���Է������
	bool ok = false;
	do
	{
		ok = push(packet,false,1);
//		JPLAYER_LOG_INFO("push return video :  %d total size：%d data size :%d",ok,size(),packet.size);
		///����ɹ����˳�
		if (ok)
		{
			break;
		}

		if(videoStreamMode == RECORD_VIDEO_STREAM_MODE)
			m_packetWaiter.timedwait(600);    //record 500
		else
			m_packetWaiter.timedwait(10);     //live 10

		if(size() == 0)            //clear all buffer
			return 0;

	} while (count--);


	///���Զ��Ҳ�Ų����룬��������֡�ˣ���֮ǰ��֡
	if (!ok)
	{
//		JPLAYER_LOG_INFO("drop frame\n");
		ok = push(packet,true,1);
	}

	return 0;
}


bool VideoManager::setSurface(void *surface)
{
#ifdef XBMC_DECODE
	if (m_pXbmcDecoder == NULL)
	{
		m_pXbmcDecoder = new(std::nothrow) XbmcDecode();
		JPLAYER_LOG_ERROR("use XbmcDecoder\n");
	}

	if (m_pXbmcDecoder != NULL)
	{
		return m_pXbmcDecoder->setSurface(surface);
	}
#endif

#ifdef OMX_DECODE
	if (m_pOmxDecoder == NULL)
	{
		m_pOmxDecoder = new(std::nothrow) JOmxDecode();
		JPLAYER_LOG_ERROR("use OmxDecoder\n");
	}

	if (m_pOmxDecoder != NULL)
	{
		return m_pOmxDecoder->setNativeWindow(surface);
	}
#endif

	return false;
}

unsigned int VideoManager::Sys_GetTickCount1(void)
{
	struct timeval aika;
	gettimeofday(&aika,NULL);
	unsigned long msecs = aika.tv_sec * 1000;
	msecs += (aika.tv_usec / 1000);
	return msecs;
}

int VideoManager::str_sunday_search(char *sur,int p_surlen, char *tar, int p_tarlen)
{
	int i = 0, j = 0, k;/*分别记录sur索引,tar索引还有,串匹配计数时索引*/
	int sl, tl;/*分别记录字符串sur和tar的长度*/
	int pe=0;/*分别记录sur匹配tar最后的索引的下一个索引*/
	int rev=0;/*记录返回的索引值,默认-1防止传入"\0"字符串*/

	/*非法情况,直接返回-1*/
	if ((!sur) || (!tar)|| p_surlen < p_tarlen)
		return -1;

	while (i<p_surlen&&j<p_tarlen)
	{
		if (sur[i] == tar[j]){/*匹配正确就仅继续匹配*/
			++i;
			++j;
			continue;
		}
		pe = rev +p_tarlen;
		/*匹配失败,移动i和j索引值,为下一次匹配做准备*/
		if (pe >= p_surlen)/*当下一次的位置已经超过text的长度时,返回-1表示没有找到*/
		{
			return -1;
		}
		for (k = p_tarlen - 1; k >= 0 && sur[pe] != tar[k]; --k)
			;

		i = rev + p_tarlen - k;//(p_tarlen - k)表示i需要移动的步长
		rev = i;//记录当前的索引
		j = 0;/*j重新开始*/
	}

	return i < p_surlen ? rev : -1;
}

int VideoManager::getFrameType(unsigned char *framebuffer,int framesize, int *frametype)
{
    int totalsize = framesize;
    int index = 0;
    char naluhead[4] = {0x00,0x00,0x00,0x01};
    while(1)
    {
        int nalusripttar = str_sunday_search((char *)framebuffer+index,totalsize,naluhead,4);
        if(nalusripttar>=0)
        {
//        	JPLAYER_LOG_INFO("str_sunday_search :%d %.2x %.2x %.2x %.2x %.2x",nalusripttar,framebuffer[nalusripttar+index],framebuffer[nalusripttar+index+1],framebuffer[nalusripttar+index+2],framebuffer[nalusripttar+index+3],framebuffer[nalusripttar+index+4]);
            char frame_type = framebuffer[nalusripttar+index+4] & 0x1f;
            if(frame_type == 0x05)
            {
                *frametype = 5;
                return 0;
            }
            else if(frame_type == 0x07||frame_type == 0x08)
            {
                totalsize-=(nalusripttar+4);
                index+=(nalusripttar+4);
                *frametype = 7;
            }
            else if(frame_type == 0x01)
            {
                *frametype = 1;
                return 0;
            }
            else
            {
            	totalsize-=(nalusripttar+4);
            	index+=(nalusripttar+4);
            }
        }
        else
            break;
    }
    return 0;
}

int VideoManager::run()
{
	while (!m_canExit)
	{
		///��ͣ����
		if (m_hasPause)
		{
			m_packetWaiter.timedwait(10);
			continue;
		}

//		int queueSize = size();
//		JPLAYER_LOG_INFO("queuesize :%d ",queueSize);
//		if(queueSize < 30)
//		{
//			m_packetWaiter.timedwait(40);
//			continue;
//		}

		///ͳ��ʱ��

//		if(smoothflag == 0)
//		{
		comn::Time tsElapse1;
//		}

		///ȡ��ݰ�
		StreamPacket h264packet;
		if (!popTimeWait(h264packet,30))
		{
//			JPLAYER_LOG_INFO(" videomanagerrun  :  %d ",h264packet.seq);
			continue;
		}

#if defined(USE_HARD_DECODER)
		///Ӳ�����������ץ�ģ�������ffmpeg����
		if (m_doSnap)
		{
			m_h264Decoder.inputData(h264packet);
		}

		int ret = -1;

#if defined(XBMC_DECODE)

		if (m_pXbmcDecoder != NULL)
		{
			ret = m_pXbmcDecoder->inputData(h264packet);
			if (ret == 0)
			{
				doHasPicCallback(h264packet.timestamp);

				int64_t lastAudioPts;
				getAvSyncManager().controlVideoPts(h264packet.timestamp,
						0,lastAudioPts);
			}
		}

#elif defined(OMX_DECODE)
		if (m_pOmxDecoder != NULL)
		{
			ret = m_pOmxDecoder->inputData(h264packet);
			if (ret == 0)
			{
				doHasPicCallback(h264packet.timestamp);

				int64_t lastAudioPts;
				getAvSyncManager().controlVideoPts(h264packet.timestamp,
						0,lastAudioPts);
			}
		}
#endif
#else

		if (!m_hasExit)
		{
			int diffDeltatime = 0;  // 缓冲区buffer时长
			int deltatime = 0;		// 当前帧延时
			int timedelay = 0;		// 动态缓冲
			int spsppsflag = 0;
			if(playFrameCount == 0)   // 第一帧  马上显示
			{
				int aframetype = 0;
				getFrameType(h264packet.data, h264packet.size, &aframetype);
//				JPLAYER_LOG_INFO("first frame type:%d \n",aframetype);
				if (aframetype == 5) {
					playFrameCount = 1;
					if (startPts == 0) startPts = h264packet.timestamp;
					if (startTime == 0) startTime = Sys_GetTickCount1();
					lastPts = h264packet.timestamp;
				}
			}
			else
			{
				if(h264packet.timestamp < lastPts && lastPts > 0xff0000 && (videoStreamMode == UDP_VIDEO_STREAM_MODE || videoStreamMode == RTMP_VIDEO_STREAM_MODE))
				{
					JPLAYER_LOG_INFO("time Rollback video \n");
					h264packet.timestamp += 0xfffffe;
				}

				int aframetype = 0;
				getFrameType(h264packet.data, h264packet.size, &aframetype);
//				JPLAYER_LOG_INFO("next frame type:%d \n",aframetype);
				if (playFrameCount == 1 && aframetype == 5) //第二帧也是I帧 ，则将基准帧置为第二帧，重新校准
				{
					startPts = h264packet.timestamp;
					startTime = Sys_GetTickCount1();
					playFrameCount++;
					lastPts = h264packet.timestamp;
				}
				else if(aframetype != 7 && aframetype !=8)
				{
					if((h264packet.timestamp>lastPts+5000 || h264packet.timestamp<lastPts-5000) && m_SeekSuccess && videoStreamMode == RECORD_VIDEO_STREAM_MODE ) //录像seek
					{
						if(aframetype != 5)
						{
							JPLAYER_LOG_INFO("seek has last P frame :%llu",h264packet.timestamp);
							release(h264packet);
							continue;
						}
						JPLAYER_LOG_INFO("change record start time :%llu",h264packet.timestamp);
						m_SeekSuccess = false;
						startPts = h264packet.timestamp;
						startTime = Sys_GetTickCount1();
						getAvSyncManager().setCurrentDelayTime(0);
					}
					else if(h264packet.timestamp>lastPts+5000 && !m_SeekSuccess && videoStreamMode == RECORD_VIDEO_STREAM_MODE)
					{
						JPLAYER_LOG_INFO("time skip on record :%llu",h264packet.timestamp);
						startPts = h264packet.timestamp;
						startTime = Sys_GetTickCount1();
						getAvSyncManager().setCurrentDelayTime(0);
					}

					diffDeltatime = (int)(m_lastpushtime - h264packet.timestamp);
					timedelay = getAvSyncManager().getCurrentDelayTime();
					deltatime = (int)((Sys_GetTickCount1() - startTime) - (h264packet.timestamp-startPts));

					if (diffDeltatime>m_MaxBufferTime)  //缓冲区时长大于最大缓冲时长，需要丢GOP组
					{
						JPLAYER_LOG_INFO("diffDeltatime>m_MaxBufferTime");
						int aframetype = 0;
						unsigned long long packettime = h264packet.timestamp;
						int droptime = 0;
						while( aframetype != 5 || m_lastpushtime - packettime >= m_MaxBufferTime)
						{
							if (h264packet.data != NULL) {
								JPLAYER_LOG_INFO("diffDeltatime>m_MaxBufferTime  release");
								droptime += (h264packet.timestamp -lastPts);
								lastPts = h264packet.timestamp;
								release(h264packet);
							}
							if (!popTimeWait(h264packet,30))
							{
								JPLAYER_LOG_INFO("diffDeltatime>m_MaxBufferTime  pop failed");
								continue;
							}
							packettime = h264packet.timestamp;
							getFrameType(h264packet.data, h264packet.size, &aframetype);
							JPLAYER_LOG_INFO("diffDeltatime>m_MaxBufferTime :%d  :%d ",aframetype,m_lastpushtime - packettime);
						}

						if(timedelay>m_MaxBufferTime)
						{
							getAvSyncManager().setDropTime(droptime,0);
							timedelay = (timedelay-droptime+99)/100*100;
							getAvSyncManager().setCurrentDelayTime(timedelay);
						}
						else
						{
							JPLAYER_LOG_INFO("start time drop :%d \n",droptime);
							getAvSyncManager().setDropTime(droptime,1);
							startTime -= droptime;
						}

						deltatime = (int)((Sys_GetTickCount1() - startTime) - (h264packet.timestamp-startPts));

						JPLAYER_LOG_INFO("diffDeltatime>m_MaxBufferTime  :  %d   %d   %d",droptime,timedelay,timedelay);
					}

					//增加缓冲区大小
					if(m_BufferEmpty)
					{
						m_BufferEmpty = false;
						if(deltatime > timedelay+100)
						{
							JPLAYER_LOG_INFO("add delta buffer time :%d  timedelay:%d",deltatime,timedelay);
							getAvSyncManager().setCurrentDelayTime((deltatime+99)/100*100);
							timedelay = getAvSyncManager().getCurrentDelayTime();
						}
					}

					if(diffDeltatime == 0) //缓冲区没数据
					{
						JPLAYER_LOG_INFO("diffDeltatime == 0");
						m_BufferEmpty = true;
					}

					int nCount = 0;
					while(Sys_GetTickCount1() - startTime < (h264packet.timestamp-startPts)+timedelay && !m_hasExit)
					{
						//							JPLAYER_LOG_INFO("Sys_GetTickCount1() :%d ",Sys_GetTickCount1() );
//						usleep(2000);
						m_decodeWaiter.timedwait(2);
						nCount ++;
						if(nCount >= 500*60)
						{
							JPLAYER_LOG_INFO("timestamp interval is too big than 60s :%d startPts:%llu framePts:%llu",timedelay,startPts,h264packet.timestamp);
							break;
						}
					}
					playFrameCount++;
					lastPts = h264packet.timestamp;
				}
				else if(aframetype == 7 || aframetype == 8)
				{
					spsppsflag = 1;
				}
			}

			comn::Time tsElapse;

//				JPLAYER_LOG_INFO(" videomanagerrun  :%llu  %d ",h264packet.timestamp,h264packet.seq);
			/*int nRet = */m_h264Decoder.inputData(h264packet);
//				JPLAYER_LOG_INFO("m_h264Decoder  inputData :  %d\n",nRet);

			release(h264packet);

			///��ʱ���룬���ƶ���
			int tsElapseTimer = tsElapse.getElapse();

			aDecodeTimelist[aDeocdeCount%20] = tsElapseTimer;
			aDeocdeCount++;
			int nDeocodeTotalTime = 0;
			for(int i=0;i<20;i++)
			{
				nDeocodeTotalTime += aDecodeTimelist[i];
			}
			int nDecodeAVGtime = nDeocodeTotalTime/((aDeocdeCount<20)?aDeocdeCount:20);

			if (m_jpMsgCallback != NULL && spsppsflag == 0)
			{
				(*m_jpMsgCallback)(m_playerHandle,JPLAYER_MSG_BUFFER_LENGTH,diffDeltatime,nDecodeAVGtime,m_msgDwUser);
				(*m_jpMsgCallback)(m_playerHandle,JPLAYER_MSG_DELAY_TIME,timedelay,deltatime,m_msgDwUser);
			}
			JPLAYER_LOG_INFO("deltatime :%d timedelay:%d decodetime :%d diffDeltatime:%d  \n ",deltatime,timedelay,nDecodeAVGtime,diffDeltatime);
			JPLAYER_LOG_INFO("firstplaycount :%d maxbuffercount:%d timedelay :%d diffDeltatime:%d deltatime :%d  \n ",m_MinBufferTime,m_MaxBufferTime,timedelay,diffDeltatime,deltatime);

			int frameInterval = m_FrameIntervalElapse.getElapse();

//				JPLAYER_LOG_INFO("deltatime decode time:%d \n",tsElapseTimer);

			int64_t lastAudioPts;
			int isAudio = 1;
			int playDelay = getAvSyncManager().controlVideoPts(h264packet.timestamp,0,lastAudioPts,isAudio);
		}

#endif

	}
	return 0;
}


void VideoManager::handleJitter(int tsElapseTimer,int timeinterval)
{
//	int decodeTimer = (int)((float)1.0/m_scale);

    ///һ֡��ʱʱ��
//	decodeTimer = 1000/DEFAULT_VIDEO_FPS/decodeTimer;

	int decodeTimer = timeinterval;

	if(videoStreamMode == RECORD_VIDEO_STREAM_MODE)
	{
		//��������ʱ��Ӧ�ü��ٲ��ţ����ܻ��������貨΢��
			int queueSize = size();
			if (queueSize >= DEFAULT_VIDEO_FPS * 10) ///������10�����
			{
				decodeTimer /= 4;    ///ÿ��60֡������������ʽ���
			}
			else if (queueSize >= DEFAULT_VIDEO_FPS * 3) ///������3�����
			{
				decodeTimer /= 2;   ///ÿ��33֡
			}
			else if (queueSize > 3)
			{
				decodeTimer /= 1.5;    //  ��������������֡��ö��г��ֻ�ѹʱ����Ѹ��׷����
			}
			else
			{
				decodeTimer -= 5;		// ���
			}
	}
	else
		decodeTimer -= 5;

//	JPLAYER_LOG_INFO("tsElapseTimer :%d decodeTimer:%d ",tsElapseTimer,decodeTimer);
	if (tsElapseTimer < decodeTimer)
	{
		tsElapseTimer = decodeTimer - tsElapseTimer;
//		m_decodeWaiter.timedwait(tsElapseTimer);
	}
}


bool  VideoManager::pause(bool hasPause)
{
	m_hasPause = hasPause;
	return true;
}

bool  VideoManager::setScale(float scale)
{
	m_scale = scale;
	return true;
}

bool VideoManager::clearAllBuffer()
{
	m_h264Decoder.flushBbuffers();
	releaseAllBuffer();
	m_yuvQueue.releaseAllBuffer();

//	getAvSyncManager().setCurrentDelayTime(1000);

	return true;
}

int  VideoManager::getYUV420PFrame(int milltimes,JPlayer_MediaFrame& frame)
{
	if (m_hasExit)
	{
		clearAllBuffer();

		return DISPLAY_PLAYER_CLOSED;
	}

	if (!m_yuvQueue.popTimeWait(frame,milltimes))
	{
		return DISPLAY_PLAYER_EMPTY;
	}

	return DISPLAY_PLAYER_OK;
}


void  VideoManager::releaseYUV420PFrame(JPlayer_MediaFrame& frame)
{
	if (frame.m_pData[0] != NULL)
	{
		m_yuvQueue.release(frame);
		frame.m_pData[0] = NULL;
	}

	return ;
}

void  VideoManager::setCurrentTime(long p_CurrentTime)
{
//	getAvSyncManager().setCurrentTime(p_CurrentTime);
	lastPts = 0;
	int64_t lastAudioPts;
	int isAudio;
	getAvSyncManager().controlVideoPts(0,1,lastAudioPts,isAudio);
//	JPLAYER_LOG_INFO("controlvideoPts  audio:%lld  isAudio:%d",lastAudioPts,isAudio);

}

void VideoManager::setBeginTime(long p_BeginTime)
{
//	getAvSyncManager().setBeginTime(p_BeginTime);
}

void VideoManager::setVideoStreamMode(int p_Mode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength)
{
	videoStreamMode = p_Mode;
	m_lastpushtime = 0;
	m_SeekSuccess = false;
	startPts = 0;
	startTime = 0;
	m_DropStreamTime = 0;
	lastPts = 0;
	int64_t lastAudioPts;
	int isAudio;
	int minBufferlen = 100;
	int maxBufferlen = 10000;
	int setDefaultBuffer = 0;
	getAvSyncManager().controlVideoPts(0,1,lastAudioPts,isAudio);

	if(videoStreamMode == UDP_VIDEO_STREAM_MODE)
	{
		getAvSyncManager().controlDefaultDropTime(100,5000);
		minBufferlen = 100;
		maxBufferlen = 5000;
	}
	else if(videoStreamMode == RTMP_VIDEO_STREAM_MODE)
	{
		getAvSyncManager().controlDefaultDropTime(1500,5000);
		minBufferlen = 1500;
		maxBufferlen = 5000;
	}

	if (p_MinMediaBufferLength >=0 && p_MinMediaBufferLength <= 5000)
	{
		setDefaultBuffer = 1;
		minBufferlen = p_MinMediaBufferLength;
	}

	if (p_MaxMediaBufferLength >=0 && p_MaxMediaBufferLength <= 10000)
	{
		setDefaultBuffer = 1;
		if(p_MaxMediaBufferLength<p_MinMediaBufferLength)
			maxBufferlen = p_MinMediaBufferLength;
		else
			maxBufferlen = p_MaxMediaBufferLength;
	}

	m_MinBufferTime = minBufferlen;
    m_MaxBufferTime = maxBufferlen;
    if(videoStreamMode == RECORD_VIDEO_STREAM_MODE)
    {
    	setDefaultBuffer = 1;
    	minBufferlen = 0;
    	maxBufferlen = 1000000000;
    	m_MinBufferTime = 0;
    	m_MaxBufferTime = 1000000000;
    }
	if (setDefaultBuffer == 1)
		getAvSyncManager().controlDefaultDropTime(minBufferlen,maxBufferlen);

	m_tsElapse.reset();
	m_FrameIntervalElapse.reset();
	m_FirstOpenFlag = 1;
	m_BufferEmpty = false;
	JPLAYER_LOG_INFO("VideoStreamMode  reset");
}

void VideoManager::seekSuccess()
{
	m_SeekSuccess = true;
}



