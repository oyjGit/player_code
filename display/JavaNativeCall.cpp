#include "NetPosaJPlayerSDK.h"
#include "MediaInfo.h"
#include "Logs.h"
#include "PlayerConfig.h"
#include "GlobalFunc.h"
#include "VideoRendererGles.h"
#include "TSingleton.h"
#include <jni.h>

//#include "PusherClient.h"
#include "cJSON.h"

#include "CloudPlatformAPI.h"
#include "ExCloudPlatformAPI.h"
 #include <unistd.h>
#include "udpSenderLog.h"

//#include "Hash_alg.h"
//#include "json/json.h"

#include "HttpLib.h"

#include "TResQueue.h"
#include "jsonsubtitute.h"
#include "Hash_alg.h"

#define URL_LENTH 60
long fromtime = 0;
long totime = 0;
int cameraCid = 0;
unsigned long long m_pos = 0;
//int locateflag = 0;
int streamMode = 1;
int privatePlayingFlag = 0;
//disc_slice_read_list_t * request_list;
char *streampath = new char[URL_LENTH];
int startpushflag = 0;

int playerOpenFlag = 0;

int messageType = 0;

JavaVM *gJavaVM;
jobject gPlayStatuJavaObj;
jobject gStreamInfoJavaObj;
jobject gMessageJavaObj;
jmethodID gPlayStatusNativeCallback;
jmethodID gStreamInfoNativeCallback;
jmethodID gMessageNativeCallback;
int nOpenPlayStatusNativeCallbackFlag = 1;
int nOpenStreamInfoNativeCallbackFlag = 0;
int firstPlayStatusFlag = 1;

float aVbpslist[30] = {0.0};
float aAbpslist[30] = {0.0};
float fStreamParamVbps = 0.0;
float fStreamParamAbps = 0.0;
float fStreamParamVfps = 0.0;
float fStreamParamAfps = 0.0;
float fStreamParamAVGVbps = 0.0;
float fStreamParamAVGAbps = 0.0;
float fStreamParamTotalVbps = 0.0;
float fStreamParamTotalAbps = 0.0;
int rtmpSFrameRate = 0;
int rtmpSBitRate = 0;
int rtmpSendTimePercent = 0;
int onlineNumber = 0;
//char m_RTMPURL[20]={0};
int nbpsCount = 0;
unsigned int rtmpPlayBeginTime = 0;

int nBufferFrameNum = 0;
int nBufferLen = 0;
int nDelayTime = 0;
int nDeltaDelayTime = 0;

int nResolutionWidth = 0;
int nResolutionHeight = 0;

static comn::CQueue<char*> queCloudMessage;
static pthread_t tidCloudMessage;
static volatile int bCloudMessageProcessing = 0;

#ifdef OMX_DECODE
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include "OmxDecode.h"
#endif

#ifdef XBMC_DECODE
#include "JNIThreading.h"
#include "XbmcDecode.h"
#endif

/////////////////////////////////////////

typedef comn::Singleton<VideoRendererOpenGles20> SingletonVideoRendererOpenGles20;

VideoRendererOpenGles20& GetVideoRendererOpenGles20()
{
	return SingletonVideoRendererOpenGles20::instance();
}



///////////////////////////////////////////
#define JDEBUG
#ifdef JDEBUG
//void testPlayer();

int   appStartCloudService(const char* aAppID, const char* aUsername, const char* aPassword, const char* aConfigString);
int   appStartCloudServiceV2(const char* aUserToken, const char* aConfigString);
void  appStopCloudService();

static void* CloudMessageHandler(void* p);

int   PopMessage(void *apData, const char *aMessage);
void  audio_consume(char* audio_data,int len);

int   ConnectPublicLive(const char *  aCameraHashID);

int   locateStream(long p_CurrentTime);

int  openVideoPlayer( int p_MinMediaBufferLength, int p_MaxMediaBufferLength);
void  closeVideoPlayer();
int   openAudioPlayer();
void  closeAudioPlayer();
int   setVolume(int p_nVolume);
int   getVolume();
void  pausePlay(const int p_PauseFlag);
void  setPlayPos(long p_lRelativePos);
int   getPlayPos();
void  setScale(float p_fScale);
float getScale();
int   openRecord(const char * p_nFileName);
void  closeRecord();
int   snapshot(const char *p_PicPath);

#endif

//锟斤拷璇诧拷锟�
int VideoManager::simpleDrawFrame()
{

	if(m_FirstDraw)
	{
		m_FirstDraw = false;
		JPLAYER_LOG_INFO("simpleDrawFrame hasExit clearDisplay m_FirstDraw");
		GetVideoRendererOpenGles20().clearDisplay();

//		if (m_lastFrame.m_pData[0] != NULL)
//		{
//			m_yuvQueue.release(m_lastFrame);
//			m_lastFrame.m_pData[0] = NULL;
//		}
//
//		clearAllBuffer();
//
//		if (--m_clearDisplauNum == 0)
//		{
//			m_decodeWaiter.post();
//		}

		return 0;
	}
	
	if (m_hasExit)
	{
//		JPLAYER_LOG_INFO("simpleDrawFrame hasExit clearDisplay");
		GetVideoRendererOpenGles20().clearDisplay();

		if (m_lastFrame.m_pData[0] != NULL)
		{
			m_yuvQueue.release(m_lastFrame);
			m_lastFrame.m_pData[0] = NULL;
		}

		clearAllBuffer();

		if (--m_clearDisplauNum == 0)
		{
			m_decodeWaiter.post();
		}


//		JPLAYER_LOG_INFO("m_hasExit ");

		return 0;

	}

//	JPLAYER_LOG_INFO("m_yuvQueue :%d ",m_yuvQueue.size());

	JPlayer_MediaFrame frame;

	bool flag= m_yuvQueue.popTimeWait(frame,VIDEO_DISPLAY_WAITTIME);
//	JPLAYER_LOG_INFO(" flag: %d m_yuvQueue seq :%d ",flag,frame.m_serialNum);
	if (flag)
	{
		///锟斤拷锟斤拷锟界��锟斤拷
		if (m_lastFrame.m_pData[0] != NULL)
		{
			m_yuvQueue.release(m_lastFrame);
			m_lastFrame.m_pData[0] = NULL;
		}

		m_lastFrame = frame;

		if(videoStreamMode == 1 || videoStreamMode == 3)
			{
				int64_t lastAudioPts;
				int isAudio;
				int playDelay = getAvSyncManager().controlVideoPts(frame.m_timestamp,0,lastAudioPts,isAudio);
				if (playDelay > 0)  ///��Ƶ��ǰ�ˣ��ȴ�һ��
				{
					//			JPLAYER_LOG_INFO("playDelay :%d ",playDelay);


					//					struct timespec slptm;
					//					slptm.tv_sec = 0;
					//					slptm.tv_nsec = 1000*1000*playDelay; //1000 ns = 1 us</p> <p> //struct sched_param param;
					//					if (nanosleep(&slptm, NULL) == -1)
					//					{
					//						perror("Failed to nanosleep");
					//						JPLAYER_LOG_INFO("Failed to nanosleep ");
					//		//				return 1;
					//					}

					//				int i;
					//				struct timespec slptm;
					//				long tdif;
					//				struct timeval tend, tstart;
					//				gettimeofday(&tstart, NULL);

					m_decodeWaiter.timedwait(playDelay);

					//				gettimeofday(&tend, NULL);
					//				tdif = 1000000L * (tend.tv_sec - tstart.tv_sec) + (tend.tv_usec - tstart.tv_usec);
					//				JPLAYER_LOG_INFO("nanosleep() time is %ld us  playDelay:%d \n", tdif/1000,playDelay);
				}
				else if (playDelay < 0)  ///��Ƶ��ǰ�ˣ������Ƶ���壬�ȴ�����ݣ������Ż�
				{
					///���ſ��
					JPLAYER_LOG_INFO("clearAllBuffer ");
					clearAllBuffer();
				}
			}
	}
	else
	{
//		JPLAYER_LOG_INFO("========== last frame ======== ");
		frame = m_lastFrame;
	}

	if (frame.m_pData[0] == NULL)
	{
//		JPLAYER_LOG_INFO(" flag: %d m_yuvQueue seq :%d  size :%d",flag,frame.m_serialNum,m_yuvQueue.size());
//		JPLAYER_LOG_INFO("frame.m_pData[0] null ");
		return 0;
	}




	if (!m_hasExit)
	{
		if(flag)
		{
//			if(locateflag == 0)
//			{
				m_pos = frame.m_timestamp - (unsigned long long)fromtime*1000;
//				JPLAYER_LOG_INFO("m_pos:%d",(int)m_pos);
//			}
//			else
//			{
//				locateflag = 0;
//				JPLAYER_LOG_INFO("m_pos: drop locate");
//			}
		}
		GetVideoRendererOpenGles20().renderYUV420Frame(frame.m_pData[0],
			frame.m_width,
			frame.m_height);
	}

	JPLAYER_LOG_INFO("deltatime decode time: get live timestamp video :%llu",frame.m_timestamp);





	return 0;

}




extern "C" 
{
	
	///new interface
//	JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_initSurface(JNIEnv * env,
//		jobject obj,
//		jobject jsurface,
//		jint width,
//		jint height);
//	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_unInitSurface(JNIEnv * env,
//		jobject obj);
//	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_draw(JNIEnv * env,
//		jobject obj);
//
//
//	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_zoominit(JNIEnv * env,
//		jobject obj,
//		jint x,jint y,jint w, jint h);
//
//	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_zoomTranslate(JNIEnv * env,
//		jobject obj, jfloat x,jfloat y,jfloat xscale,jfloat yscale);
//
//	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_zoom(JNIEnv * env,
//		jobject obj, jfloat x,jfloat y);
//
//	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Translate(JNIEnv * env,
//		jobject obj, jfloat x,jfloat y);

/*============平台接口=============*/
	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectAP(JNIEnv * env,
		jobject obj,jstring aUUID,jstring aPassword,jint aWifiType);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectDeviceAP(JNIEnv * env,
		jobject obj,jstring aUUID,jstring aPassword,jint aWifiType,jstring aHashID);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_UpdateStatus(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetOnlineStatus(JNIEnv * env,
		jobject obj);

	JNIEXPORT jfloat JNICALL Java_com_yang_sdk_api_PlatformAPI_GetPlatformVersion(JNIEnv * env,
		jobject obj);

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetUserAccessToken(JNIEnv * env,
		jobject obj);

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetDeviceAccessToken(JNIEnv * env,
		jobject obj,jstring aCid);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetUserTokenValueCount(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetDeviceTokenValueCount(JNIEnv * env,
		jobject obj,jstring aCid);

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_DeviceBound(JNIEnv * env,
		jobject obj,jstring aSN);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_DeviceUnBound(JNIEnv * env,
		jobject obj,jstring aCid);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetDeviceStatus(JNIEnv * env,
		jobject obj,jstring aCid);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartCloudService(JNIEnv * env,
		jobject obj,jstring aAppID,jstring aUsername, jstring aPassword, jstring aConfigString);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartCloudServiceV2(JNIEnv * env,
		jobject obj,jstring aUserToken, jstring aConfigString);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_StopCloudService(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPrivateLive(JNIEnv * env,
		jobject obj,jstring aCid);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectP2PLive(JNIEnv * env,
		jobject obj,jstring aAccessToken,jstring aTrackIP,jint aTrackPort);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPublicLive(JNIEnv * env,
		jobject obj,jstring aCid,jstring aRTMPURL);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectRTMPLive(JNIEnv * env,
		jobject obj,jstring aRTMPURL);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPrivateLiveEx(JNIEnv * env,
		jobject obj,jstring aCid, jint aMinMediaBufferLength, jint aMaxMediaBufferLength);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPublicLiveEx(JNIEnv * env,
		jobject obj,jstring aCid,jstring aRTMPURL, jint aMinMediaBufferLength, jint aMaxMediaBufferLength);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_Disconnect(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_SendMessage(JNIEnv * env,
		jobject obj,jstring aCid,jstring aMessage);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordSource(JNIEnv * env,
		jobject obj,jstring aDiskInfo,jstring aCid,jlong aFrom,jlong aTo,jlong aPlay);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordSourceV2(JNIEnv * env,
		jobject obj,jstring aAccessToken,jstring aDiskInfo,jlong aFrom,jlong aTo,jlong aPlay);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordTest(JNIEnv * env,
		jobject obj,jstring aDiskInfo,jstring aCid,jlong aFrom,jlong aTo,jlong aPlay);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordTestV2(JNIEnv * env,
		jobject obj,jstring aAccessToken,jlong aFrom,jlong aTo,jlong aPlay);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_Seek(JNIEnv * env,
		jobject obj,jlong aCurrentTime);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_SendVideoBuffer(JNIEnv * env,
		jobject obj,jobject aBuffer,jint aLen,jint aRotation,jlong aTimeStamp,jint aVideoType);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_SendAudioBuffer(JNIEnv * env,
		jobject obj,jbyteArray aBuffer,jint aLen,jlong aTimeStamp);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartTalking(JNIEnv * env,
			jobject obj, jint samplerate, jint channels, jint bitsize, jint delay);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_StopTalking(JNIEnv * env, jobject obj);

/*============手机直播=============*/

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetConnectionString(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartConnectToClient(JNIEnv * env,
		jobject obj,jstring aConnectionConfigInfo);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartConnectToServer(JNIEnv * env,
		jobject obj,jint aMode);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_InitStreamEncoder(JNIEnv * env,
		jobject obj,jint aWidth,jint aHeight,jint aBitrate);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_UninitStreamEncoder(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ProcessCameraPreview(JNIEnv* env, jobject obj,
	    jint width, jint height, jint rotation, jint inFormat, jint outFormat, jobject inBuffer, jobject outBuffer);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_PostCoverPicture(JNIEnv * env,
		jobject obj,jbyteArray aBuffer,jint aLen);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_MessageNativeCallback(JNIEnv * env,
		jobject obj);

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetStatus(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartDownloadRecord(JNIEnv * env,
		jobject obj,jstring aDiskInfo,jstring aCid,jlong aFrom,jlong aTo,jstring aDownloadFile);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_StopDownloadRecord(JNIEnv * env,
		jobject obj);




/*============播放接口=============*/
	JNIEXPORT void 	  JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_PlayStatusNativeCallback(JNIEnv * env,
		jobject obj,jint aOpenFlag);

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_getStreamMediaParam(JNIEnv * env,
		jobject obj,jint aParamType);

	JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_getDebugInfo(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_InitSurface(JNIEnv * env,
			jobject obj,
			jobject aSurface,
			jint aWidth,
			jint aHeight);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_UninitSurface(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_OpenVideoPlayer(JNIEnv * env,
		jobject obj);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_CloseVideoPlayer(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_OpenAudioPlayer(JNIEnv * env,
		jobject obj);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_CloseAudioPlayer(JNIEnv * env,
		jobject obj);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_DrawVideo(JNIEnv * env,
		jobject obj);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_GetPlayPos(JNIEnv * env,
		jobject obj);

	JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_OpenRecord(JNIEnv * env,
		jobject obj, jstring aFileName);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_CloseRecord(JNIEnv * env,
		jobject obj);

	JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Snapshot(JNIEnv * env,
		jobject obj, jstring aFileName);


/*============待实现=============*/
	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_SetVolume(JNIEnv * env,
		jobject obj, jint aVolume);

	JNIEXPORT jint JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_GetVolume(JNIEnv * env,
		jobject obj);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Pause(JNIEnv * env,
		jobject obj);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Play(JNIEnv * env,
		jobject obj);

	JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_SetScale(JNIEnv * env,
		jobject obj, jfloat aScale);

	JNIEXPORT jfloat JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_GetScale(JNIEnv * env,
		jobject obj);

};





//
//JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_zoominit(JNIEnv * env,
//																  jobject obj,
//																  jint x,
//																  jint y,
//																  jint w,
//																  jint h)
//{
//#ifdef TEST_MODEL_MATRIX
//	GetVideoRendererOpenGles20().matrixLoadIdentity();
//#endif
//
//#ifdef TEST_TEXT_MATRIX
//	GetVideoRendererOpenGles20().textureMatrixLoadIdentity();
//#else
//	GetVideoRendererOpenGles20().setViewport(x,y,w,h);
//#endif
//
//	getVideoManager().signalYUVQueue();
//
//}
//
//
//JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_zoomTranslate(JNIEnv * env,
//																	   jobject obj,
//																	   jfloat x,
//																	   jfloat y,
//																	   jfloat xscale,
//																	   jfloat yscale)
//{
//
//#ifdef TEST_MODEL_MATRIX
//	GetVideoRendererOpenGles20().matrixScale(x,y);
//#endif
//
//#ifdef TEST_TEXT_MATRIX
//	GetVideoRendererOpenGles20().textureMatrixScaleTranslate(xscale,yscale,x,y);
//#endif
//
//}
//
//
//JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_zoom(JNIEnv * env,
//															  jobject obj,
//															  jfloat x,
//															  jfloat y)
//{
//#ifdef TEST_TEXT_MATRIX
//	GetVideoRendererOpenGles20().textureMatrixScale(x,y);
//
//	getVideoManager().signalYUVQueue();
//#endif
//}
//
//JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Translate(JNIEnv * env,
//																   jobject obj,
//																   jfloat x,
//																   jfloat y)
//{
//#ifdef TEST_TEXT_MATRIX
//	GetVideoRendererOpenGles20().textureMatrixTranslate(x,y);
//
//	getVideoManager().signalYUVQueue();
//#endif
//
//}



#ifdef XBMC_DECODE
jobject g_surface = NULL;
#endif

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectAP(JNIEnv * env,
		jobject obj,jstring aUUID,jstring aPassword,jint aWifiType)
{
	const char* sUUID = (env)->GetStringUTFChars(aUUID,0);

	if(sUUID == NULL)
		return;

	const char* sPassword = (env)->GetStringUTFChars(aPassword,0);

	int nRet = -1;
	jstring jstatus;
	if(sPassword != NULL)
	{
		char *ap = ConnectAP(sUUID,sPassword,aWifiType);
		if(ap == NULL)
			jstatus=env->NewStringUTF("");
		else
			jstatus=env->NewStringUTF(ap);

		(env)->ReleaseStringUTFChars(aPassword, sPassword);
	}
	(env)->ReleaseStringUTFChars(aUUID, sUUID);
	return jstatus;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectDeviceAP(JNIEnv * env,
	jobject obj,jstring aUUID,jstring aPassword,jint aWifiType,jstring aHashID)
{
	const char* sUUID = (env)->GetStringUTFChars(aUUID,0);

	if(sUUID == NULL)
		return;

	const char* sPassword = (env)->GetStringUTFChars(aPassword,0);

	int nRet = -1;
	if(sPassword != NULL)
	{
		const char* sHashID = (env)->GetStringUTFChars(aHashID,0);

		if(sHashID != NULL)
		{
			nRet = ConnectDeviceAP(sUUID,sPassword,aWifiType,sHashID);

			(env)->ReleaseStringUTFChars(aHashID, sHashID);
		}
		(env)->ReleaseStringUTFChars(aPassword, sPassword);
	}
	(env)->ReleaseStringUTFChars(aUUID, sUUID);
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_UpdateStatus(JNIEnv * env,
	jobject obj)
{
	return ExUpdateStatus();
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetOnlineStatus(JNIEnv * env,
	jobject obj)
{
	return GetOnlineStatus();
}

JNIEXPORT jfloat JNICALL Java_com_yang_sdk_api_PlatformAPI_GetPlatformVersion(JNIEnv * env,
	jobject obj)
{
	//待实现
	return GetPlatformVersion();
}

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetUserAccessToken(JNIEnv * env,
	jobject obj)
{
	jstring jUsertoken;
	char sUsertoken[100];
	memset(sUsertoken,0,100);
	ExGetUserToken(sUsertoken);
	jUsertoken=env->NewStringUTF(sUsertoken);
	return jUsertoken;
}

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetDeviceAccessToken(JNIEnv * env,
	jobject obj,jstring aCid)
{
	jstring jdevicetoken;
	const char* scid = (env)->GetStringUTFChars(aCid,0);
	if(scid == NULL)
	{
		jdevicetoken=env->NewStringUTF("");
		return jdevicetoken;
	}
	char sDevicetoken[100];
	memset(sDevicetoken,0,100);
	ExGetDeviceAccessToken(scid,sDevicetoken);
	jdevicetoken=env->NewStringUTF(sDevicetoken);
	(env)->ReleaseStringUTFChars(aCid, scid);
	return jdevicetoken;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetUserTokenValueCount(JNIEnv * env,
	jobject obj)
{
	return ExGetUserTokenValueCount();
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetDeviceTokenValueCount(JNIEnv * env,
	jobject obj,jstring aCid)
{
	int nTokenCount = 0;
	const char* scid = (env)->GetStringUTFChars(aCid,0);
	if(scid == NULL)
	{
		return 0;
	}
	nTokenCount = ExGetDeviceTokenValueCount(scid);
	(env)->ReleaseStringUTFChars(aCid, scid);
	return nTokenCount;
}

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_DeviceBound(JNIEnv * env,
		jobject obj,jstring aSN)
{
	const char* sn = (env)->GetStringUTFChars(aSN,0);
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_DeviceBound  %d  %s\n",strlen(sn),sn);

	int nRet = -1;
	char cid[20] = {0};
	jstring jstatus;
	if(sn != NULL)
	{
//		nRet = A_DeviceBound(sn,cid);
//		if(nRet == 0)
//		{
////			JPLAYER_LOG_INFO("appGetStatus :%s",appGetStatus());
//			jstatus=env->NewStringUTF(cid);
//		}
//		else
//		{
//			sprintf(cid,"%d",nRet);
//		}
//		jstatus=env->NewStringUTF(cid);
//
//		(env)->ReleaseStringUTFChars(aSN, sn);
	}
	return jstatus;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_DeviceUnBound(JNIEnv * env,
		jobject obj,jstring aCameraID)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_DeviceUnBound \n");
	const char* scid = (env)->GetStringUTFChars(aCameraID,0);
	int nRet = -1;
	if(scid != NULL)
	{
//		nRet = A_DeviceUnBound(scid);
//
//		(env)->ReleaseStringUTFChars(aCameraID, scid);
	}
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_GetDeviceStatus(JNIEnv * env,
		jobject obj,jstring aCid)
{
	const char* cid = (env)->GetStringUTFChars(aCid,0);
	int nRet = -1;
	if(cid != NULL)
	{
		nRet = ExGetDeviceStatus(cid);
		(env)->ReleaseStringUTFChars(aCid, cid);
	}
//	jstring jstatus;
//
//	char status[5000]={0};
//	appGetStatus(status);
//	jstatus=env->NewStringUTF(status);
//
//	return jstatus;
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartCloudService(JNIEnv * env,
		jobject obj,jstring aAppID,jstring aUsername, jstring aPassword, jstring aConfigString)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_StartCloudService");

	const char* configString = (env)->GetStringUTFChars(aConfigString,0);

	int nRet = -1;
	if(configString != NULL)
	{
		const char* appidString = (env)->GetStringUTFChars(aAppID,0);

		if(appidString != NULL)
		{
			const char* usernameString = (env)->GetStringUTFChars(aUsername,0);

			if(usernameString != NULL)
			{
				const char* passwordString = (env)->GetStringUTFChars(aPassword,0);

				if(passwordString != NULL)
				{
					nRet = appStartCloudService(appidString,usernameString,passwordString,configString);

					(env)->ReleaseStringUTFChars(aPassword, passwordString);
				}
				(env)->ReleaseStringUTFChars(aUsername, usernameString);
			}
			(env)->ReleaseStringUTFChars(aAppID, appidString);
		}

		(env)->ReleaseStringUTFChars(aConfigString, configString);
	}

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartCloudServiceV2(JNIEnv * env,
	jobject obj,jstring aUserToken, jstring aConfigString)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_StartCloudServiceV2");

	const char* configString = (env)->GetStringUTFChars(aConfigString,0);

	int nRet = -1;
	if(configString != NULL)
	{
		const char* userToken = (env)->GetStringUTFChars(aUserToken,0);

		if(userToken != NULL)
		{
			nRet = appStartCloudServiceV2(userToken,configString);
			(env)->ReleaseStringUTFChars(aUserToken, userToken);
		}

		(env)->ReleaseStringUTFChars(aConfigString, configString);
	}

	return nRet;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_StopCloudService(JNIEnv * env,
		jobject obj)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_StopCloudService \n");
	return appStopCloudService();
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPrivateLive(JNIEnv * env,
		jobject obj,jstring aCid)
{
	const char* mediaSourceIndex = (env)->GetStringUTFChars(aCid,0);
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_ConnectPrivateLiveEx  %d  %s\n",strlen(mediaSourceIndex),mediaSourceIndex);

	int nRet = -1;
	if(mediaSourceIndex != NULL)
	{
		nRet = ExConnectPrivateLive(mediaSourceIndex);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			if(nRet != 0)
				Disconnect();
			else
			{
				privatePlayingFlag = 1;
				playerOpenFlag = 1;
			}
		}
		(env)->ReleaseStringUTFChars(aCid, mediaSourceIndex);
	}
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectP2PLive(JNIEnv * env,
		jobject obj,jstring aAccessToken,jstring aTrackIP,jint aTrackPort)
{
	const char* accesstoken = (env)->GetStringUTFChars(aAccessToken,0);
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_ConnectP2PLive  %d  %s\n",strlen(accesstoken),accesstoken);

	int nRet = -1;
	if(accesstoken != NULL)
	{
		const char* sTrackIP = (env)->GetStringUTFChars(aTrackIP,0);

		if(sTrackIP != NULL)
		{
			nRet = ConnectP2PLive(accesstoken,sTrackIP,aTrackPort);
			if(nRet == 0)
			{
				nRet = openVideoPlayer(-1,-1);
				if(nRet != 0)
					Disconnect();
				else
				{
					privatePlayingFlag = 1;
					playerOpenFlag = 1;
				}
			}
			(env)->ReleaseStringUTFChars(aTrackIP, sTrackIP);
		}
		(env)->ReleaseStringUTFChars(aAccessToken, accesstoken);
	}
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPublicLive(JNIEnv * env,
	jobject obj,jstring aCid,jstring aRTMPURL)
{
	const char* sCid = (env)->GetStringUTFChars(aCid,0);
	if(sCid == NULL)
	{
		return -1;
	}

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_ConnectPublicLive  %d  %s\n",strlen(sCid),sCid);

	const char* sRTMPUrl = (env)->GetStringUTFChars(aRTMPURL,0);
	int nRet = -1;
	if(sRTMPUrl != NULL)
	{
//		char * url = new char [URL_LENTH*2];
//		memset(url,0,URL_LENTH*2);
//		sprintf(url,"http://120.26.65.144:8005/API/rtmp/%s/addr", sCid);//shenzhen
//		JPLAYER_LOG_INFO("httpGet rtmp :%s",url);
//		char *response = NULL;
//		int responselen = 0;
//		int nRet  = httpGet(url,&response,&responselen,20);
//		if(nRet == 0)
//			JPLAYER_LOG_INFO("httpGet  sucess :%s ",response);
//		else
//		{
//			JPLAYER_LOG_INFO("httpGet  failed :%d ",nRet );
//			char errorlog[50];
//			sprintf(errorlog,"ConnectPublicLive get rtmpurl failed:%d",nRet);
////			sendLog(errorlog);
//			return -1;
//		}

		nRet = ExConnectPublicLive(sCid,sRTMPUrl);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
		(env)->ReleaseStringUTFChars(aRTMPURL, sRTMPUrl);
	}
	(env)->ReleaseStringUTFChars(aCid, sCid);
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectRTMPLive(JNIEnv * env,
		jobject obj,jstring aRTMPURL)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_ConnectRTMPLive\n");

	const char* sRTMPUrl = (env)->GetStringUTFChars(aRTMPURL,0);
	int nRet = -1;
	if(sRTMPUrl != NULL)
	{
		nRet = ConnectRTMPLive(sRTMPUrl);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
		(env)->ReleaseStringUTFChars(aRTMPURL, sRTMPUrl);
	}
	return nRet;
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPrivateLiveEx(JNIEnv * env,
		jobject obj,jstring aCid, jint aMinMediaBufferLength, jint aMaxMediaBufferLength)
{
	const char* mediaSourceIndex = (env)->GetStringUTFChars(aCid,0);
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_ConnectPrivateLiveEx  %d  %s\n",strlen(mediaSourceIndex),mediaSourceIndex);

	int nRet = -1;
	if(mediaSourceIndex != NULL)
	{
		nRet = ExConnectPrivateLive(mediaSourceIndex);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(aMinMediaBufferLength,aMaxMediaBufferLength);
			if(nRet != 0)
				Disconnect();
			else
			{
				privatePlayingFlag = 1;
				playerOpenFlag = 1;
			}
		}
		(env)->ReleaseStringUTFChars(aCid, mediaSourceIndex);
	}
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ConnectPublicLiveEx(JNIEnv * env,
	jobject obj,jstring aCid,jstring aRTMPURL, jint aMinMediaBufferLength, jint aMaxMediaBufferLength)
{
	const char* sCid = (env)->GetStringUTFChars(aCid,0);
	if(sCid == NULL)
	{
		return -1;
	}

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_ConnectPublicLive  %d  %s\n",strlen(sCid),sCid);

	const char* sRTMPUrl = (env)->GetStringUTFChars(aRTMPURL,0);
	int nRet = -1;
	if(sRTMPUrl != NULL)
	{
//		char * url = new char [URL_LENTH*2];
//		memset(url,0,URL_LENTH*2);
//		sprintf(url,"http://120.26.65.144:8005/API/rtmp/%s/addr", sCid);//shenzhen
//		JPLAYER_LOG_INFO("httpGet rtmp :%s",url);
//		char *response = NULL;
//		int responselen = 0;
//		int nRet  = httpGet(url,&response,&responselen,20);
//		if(nRet == 0)
//			JPLAYER_LOG_INFO("httpGet  sucess :%s ",response);
//		else
//		{
//			JPLAYER_LOG_INFO("httpGet  failed :%d ",nRet );
//			char errorlog[50];
//			sprintf(errorlog,"ConnectPublicLive get rtmpurl failed:%d",nRet);
////			sendLog(errorlog);
//			return -1;
//		}

		nRet = ExConnectPublicLive(sCid,sRTMPUrl);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(aMinMediaBufferLength,aMaxMediaBufferLength);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
		(env)->ReleaseStringUTFChars(aRTMPURL, sRTMPUrl);
	}
	(env)->ReleaseStringUTFChars(aCid, sCid);
	return nRet;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_Disconnect(JNIEnv * env,
	jobject obj)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_Disconnect \n");
	playerOpenFlag = 0;
	privatePlayingFlag = 0;
	if(getStreamMode() == 2)
	{
		closeVideoPlayer();
		Disconnect();
	}
	else
	{
		Disconnect();
		JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_Disconnect finish \n");
		closeVideoPlayer();
		JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_Disconnect close video play\n");
	}

}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordSource(JNIEnv * env,
	jobject obj,jstring aDiskInfo,jstring aCid,jlong aFrom,jlong aTo,jlong aPlay)
{

	int nRet = -1;

	fromtime = aFrom;
	totime = aTo;
	const char* sSCid = (env)->GetStringUTFChars(aCid,0);
	if(sSCid == NULL)
		return -1;

	const char* sDiskInfo = (env)->GetStringUTFChars(aDiskInfo,0);
	if(sDiskInfo != NULL)
	{
		nRet = ExQueryRecordSource(sDiskInfo,sSCid, aFrom, aTo, aPlay);
		JPLAYER_LOG_INFO("QueryRecordSource :%d",nRet);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			JPLAYER_LOG_INFO("openVideoPlayer :%d",nRet);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
		JPLAYER_LOG_INFO("ReleaseStringUTFChars aDiskInfo");
		(env)->ReleaseStringUTFChars(aDiskInfo, sDiskInfo);
	}
	JPLAYER_LOG_INFO("ReleaseStringUTFChars aCid");

	(env)->ReleaseStringUTFChars(aCid, sSCid);

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_QueryRecordSource :%d  %d  %d  %d",fromtime,totime,aPlay,nRet);

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordSourceV2(JNIEnv * env,
	jobject obj,jstring aAccessToken,jstring aDiskInfo,jlong aFrom,jlong aTo,jlong aPlay)
{
	int nRet = -1;

	fromtime = aFrom;
	totime = aTo;
	const char* accessToken = (env)->GetStringUTFChars(aAccessToken,0);
	if(accessToken == NULL)
		return -1;

	const char* sDiskInfo = (env)->GetStringUTFChars(aDiskInfo,0);
	if(sDiskInfo != NULL)
	{
		nRet = QueryRecordSourceV2(accessToken,sDiskInfo, aFrom, aTo, aPlay);
		JPLAYER_LOG_INFO("QueryRecordSourceV2 :%d",nRet);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			JPLAYER_LOG_INFO("openVideoPlayer :%d",nRet);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
//		JPLAYER_LOG_INFO("ReleaseStringUTFChars aDiskInfo");
		(env)->ReleaseStringUTFChars(aDiskInfo, sDiskInfo);
	}
//	JPLAYER_LOG_INFO("ReleaseStringUTFChars aAccessToken");

	(env)->ReleaseStringUTFChars(aAccessToken, accessToken);

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_QueryRecordSourceV2 :%d  %d  %d  %d",fromtime,totime,aPlay,nRet);

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordTest(JNIEnv * env,
	jobject obj,jstring aDiskInfo,jstring aCid,jlong aFrom,jlong aTo,jlong aPlay)
{

	int nRet = -1;

	fromtime = aFrom;
	totime = aTo;
	const char* sSCid = (env)->GetStringUTFChars(aCid,0);
	if(sSCid == NULL)
		return -1;

	const char* sDiskInfo = (env)->GetStringUTFChars(aDiskInfo,0);
	if(sDiskInfo != NULL)
	{
		nRet = ExQueryRecordTest(sDiskInfo,sSCid, aFrom, aTo,aPlay);
		JPLAYER_LOG_INFO("QueryRecordSource :%d",nRet);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			JPLAYER_LOG_INFO("openVideoPlayer :%d",nRet);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
		JPLAYER_LOG_INFO("ReleaseStringUTFChars aDiskInfo");
		(env)->ReleaseStringUTFChars(aDiskInfo, sDiskInfo);
	}
	JPLAYER_LOG_INFO("ReleaseStringUTFChars aCid");

	(env)->ReleaseStringUTFChars(aCid, sSCid);

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_QueryRecordSource :%d  %d  %d",fromtime,totime,nRet);

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_QueryRecordTestV2(JNIEnv * env,
	jobject obj,jstring aAccessToken,jlong aFrom,jlong aTo,jlong aPlay)
{
	int nRet = -1;

	fromtime = aFrom;
	totime = aTo;

	const char* sAccessToken = (env)->GetStringUTFChars(aAccessToken,0);
	if(sAccessToken != NULL)
	{
		nRet = QueryRecordTestV2(sAccessToken, aFrom, aTo,aPlay);
		JPLAYER_LOG_INFO("QueryRecordSource :%d",nRet);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			JPLAYER_LOG_INFO("openVideoPlayer :%d",nRet);
			if(nRet != 0)
				Disconnect();
			else
				playerOpenFlag = 1;
		}
		JPLAYER_LOG_INFO("ReleaseStringUTFChars aAccessToken");
		(env)->ReleaseStringUTFChars(aAccessToken, sAccessToken);
	}

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_QueryRecordSource :%d  %d  %d",fromtime,totime,nRet);

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_SendMessage(JNIEnv * env,
	jobject obj,jstring aCid,jstring aMessage)
{
	const char* sCid = (env)->GetStringUTFChars(aCid,0);
	if(sCid == NULL)
	{
		return -1;
	}
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_SendMessage  %d  %s\n",strlen(sCid),sCid);

//	const char* sMessage = (env)->GetStringUTFChars(aMessage,0);
//	int nRet = -1;
//	if(sMessage != NULL)
//	{
//		nRet = PushMessage(sCid,sMessage);
//		(env)->ReleaseStringUTFChars(aCid, sCid);
//		(env)->ReleaseStringUTFChars(aMessage, sMessage);
//		return nRet;
//	}
	return -1;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_Seek(JNIEnv * env,
	jobject obj,jlong aCurrentTime)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_LocateStream\n");

	int nRet = locateStream(aCurrentTime);
	if(nRet == 0)
	{
		m_pos = aCurrentTime*1000;
	//	locateflag = 1;
		firstPlayStatusFlag = 1;
	}

//	JPLAYER_LOG_INFO("m_pos:  locate %d",(int)m_pos);
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_SendVideoBuffer(JNIEnv * env,
	jobject obj,jobject aBuffer,jint aLen,jint aRotation,jlong aTimeStamp,jint aVideoType)
{
	int nRet = -1;
	void* srcbuf = env->GetDirectBufferAddress(aBuffer);
	if (srcbuf == NULL) {
		return -1;
	}

//	nRet = PushYUVData(pBuffer,aLen,aRotation,aTimeStamp);
	nRet = PushDataV2(srcbuf, aLen,aRotation, aTimeStamp, aVideoType);

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_SendAudioBuffer(JNIEnv * env,
		jobject obj,jbyteArray aBuffer,jint aLen,jlong aTimeStamp)
{
	int nRet = -1;
	unsigned char * pBuffer = (env)->GetByteArrayElements(aBuffer,NULL);
	if(pBuffer == NULL)
	{
		JPLAYER_LOG_INFO("GetByteArrayElements Failed!");
		return -1;
	}

	nRet = PushData(pBuffer,aLen,aTimeStamp,11);

	(env)->ReleaseByteArrayElements(aBuffer,pBuffer,0);

	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartTalking(JNIEnv * env,
			jobject obj, jint samplerate, jint channels, jint bitsize, jint delay)
{
	return StartTalking((int)samplerate, (int)channels, (int)bitsize, (int)delay);
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_StopTalking(JNIEnv * env, jobject obj)
{
	StopTalking();
}

/*============手机直播=============*/

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetConnectionString(JNIEnv * env,
	jobject obj)
{
	jstring sConnectConfig;
	char connectString[256] = {0};
	ExGetConnectionString(connectString);
	sConnectConfig=env->NewStringUTF(connectString);
	privatePlayingFlag = 1;
	return sConnectConfig;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartConnectToClient(JNIEnv * env,
	jobject obj,jstring aConnectionConfigInfo)
{
	const char* sConnectionInfo = (env)->GetStringUTFChars(aConnectionConfigInfo,0);
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_StartConnectToClient  %d  %s\n",strlen(sConnectionInfo),sConnectionInfo);

	int nRet = -1;
	if(sConnectionInfo != NULL)
	{
		nRet = ConnectToClient(sConnectionInfo);
		if(nRet == 0)
		{
			nRet = openVideoPlayer(-1,-1);
			if(nRet != 0)
				Disconnect();
			else
			{
				privatePlayingFlag = 1;
				playerOpenFlag = 1;
			}
		}
		(env)->ReleaseStringUTFChars(aConnectionConfigInfo, sConnectionInfo);
	}
	return nRet;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartConnectToServer(JNIEnv * env,
	jobject obj,jint aMode)
{
	privatePlayingFlag = 1;
	return ExConnectToServer(aMode);
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_InitStreamEncoder(JNIEnv * env,
	jobject obj,jint aWidth,jint aHeight,jint aBitrate)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_InitStreamEncoder");

	int openflag = InitEncoder(aWidth,aHeight,aBitrate);
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_InitStreamEncoder openflag :%d ",openflag);

	return openflag;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_UninitStreamEncoder(JNIEnv * env,
	jobject obj)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_PlatformAPI_UninitStreamEncoder ");
	UninitEncoder();
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_ProcessCameraPreview(JNIEnv* env, jobject obj,
	jint width, jint height, jint rotation, jint inFormat, jint outFormat, jobject inBuffer, jobject outBuffer)
{
    void* srcbuf = env->GetDirectBufferAddress(inBuffer);
    if (srcbuf == NULL) {
        return -1;
    }
    void* dstbuf = env->GetDirectBufferAddress(outBuffer);
    if (dstbuf == NULL) {
        return -1;
    }
	return ProcessCameraPreview(width, height, rotation, inFormat, outFormat, srcbuf, dstbuf);
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_PostCoverPicture(JNIEnv * env,
	jobject obj,jbyteArray aBuffer,jint aLen)
{
	int nRet = -1;
	unsigned char * pBuffer = (env)->GetByteArrayElements(aBuffer,NULL);
	if(pBuffer == NULL)
	{
		JPLAYER_LOG_INFO("GetByteArrayElements Failed!");
		return -1;
	}

	nRet = PostCoverPicture(pBuffer,aLen);

	(env)->ReleaseByteArrayElements(aBuffer,pBuffer,0);

	return nRet;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_MessageNativeCallback(JNIEnv * env,
	jobject obj)
{
	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_MessageNativeCallback");
	env->GetJavaVM(&gJavaVM);
	gMessageJavaObj = env->NewGlobalRef(obj);
	jclass objclass = env->GetObjectClass(obj);
	if(objclass == 0)
	{
		JPLAYER_LOG_INFO("MessageNativeCallback GetObjectClass failed");
		return;
	}
	gMessageNativeCallback = env->GetMethodID(objclass, "MessageNativeCallback", "(Ljava/lang/String;)V");
	if(gMessageNativeCallback == 0)
	{
		JPLAYER_LOG_INFO("MessageNativeCallback GetMethodID failed");
		return;
	}

	if (bCloudMessageProcessing == 0)
	{
		bCloudMessageProcessing = 1;
		pthread_create(&tidCloudMessage, NULL, CloudMessageHandler, NULL);
	}
	return;
}

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_PlatformAPI_GetStatus(JNIEnv * env,
	jobject obj)
{
	char cStatus[4000] = {0};
	jstring jstatus;

	int nRet = GetStatus(cStatus);

	jstatus=env->NewStringUTF(cStatus);

	return jstatus;
}

JNIEXPORT jint JNICALL Java_com_yang_sdk_api_PlatformAPI_StartDownloadRecord(JNIEnv * env,
	jobject obj,jstring aDiskInfo,jstring aCid,jlong aFrom,jlong aTo,jstring aDownloadFile)
{
	int nRet = -1;

	const char* sSCid = (env)->GetStringUTFChars(aCid,0);
	if(sSCid == NULL)
		return -1;

	const char* sDiskInfo = (env)->GetStringUTFChars(aDiskInfo,0);
	if(sDiskInfo != NULL)
	{
		const char* sDownloadFile = (env)->GetStringUTFChars(aDownloadFile,0);
		if(sDownloadFile != NULL)
		{
			nRet = ExStartDownloadRecord(sDiskInfo,sSCid, aFrom, aTo,sDownloadFile);
			JPLAYER_LOG_INFO("StartDownloadRecord :%d",nRet);
			(env)->ReleaseStringUTFChars(aDownloadFile, sDownloadFile);
		}
		(env)->ReleaseStringUTFChars(aDiskInfo, sDiskInfo);
	}
	(env)->ReleaseStringUTFChars(aCid, sSCid);

	return nRet;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_PlatformAPI_StopDownloadRecord(JNIEnv * env,
	jobject obj)
{
	StopDownloadRecord();
}



JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_getStreamMediaParam(JNIEnv * env,
	jobject obj,jint aParamType)
{
//	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_getStreamMediaParam type :%d",aParamType);
	char cParam[10] ={0};
	jstring jstrOnlineStatus;
	if(aParamType == 1)
	{
		int nParam = (int)fStreamParamVbps;
		sprintf(cParam,"%d",nParam);
//		JPLAYER_LOG_INFO("fStreamParamVbps :%s",cParam);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 2)
	{
		int nParam = (int)fStreamParamAbps;
		sprintf(cParam,"%d",nParam);
//		JPLAYER_LOG_INFO("fStreamParamAbps :%s",cParam);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 3)
	{
		int nParam = (int)fStreamParamVfps;
		sprintf(cParam,"%d",nParam);
//		JPLAYER_LOG_INFO("fStreamParamVfps :%s",cParam);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 4)
	{
		int nParam = (int)fStreamParamAfps;
		sprintf(cParam,"%d",nParam);
//		JPLAYER_LOG_INFO("fStreamParamAfps :%s",cParam);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 5)
	{
		int nParam = (int)fStreamParamAVGVbps;
		sprintf(cParam,"%d",nParam);
//		JPLAYER_LOG_INFO("fStreamParamVbps :%s",cParam);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 6)
	{
		int nParam = (int)fStreamParamAVGAbps;
		sprintf(cParam,"%d",nParam);
//		JPLAYER_LOG_INFO("fStreamParamVbps :%s",cParam);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 7)
	{
		char rtmpurl[50]={0};
		getRTMPURL(rtmpurl);
		jstrOnlineStatus=env->NewStringUTF(rtmpurl);
	}
	else if(aParamType == 8)
	{
		sprintf(cParam,"%d",rtmpSFrameRate);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 9)
	{
		sprintf(cParam,"%d",rtmpSBitRate);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 10)
	{
		sprintf(cParam,"%d",nBufferLen);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 11)
	{
		sprintf(cParam,"%d",nBufferFrameNum);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 12)
	{
		sprintf(cParam,"%d",nDelayTime);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 13)
	{
		sprintf(cParam,"%d",nDeltaDelayTime);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 14)
	{
		sprintf(cParam,"%d",nResolutionWidth);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 15)
	{
		sprintf(cParam,"%d",nResolutionHeight);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 16)
	{
		sprintf(cParam,"%d",rtmpSendTimePercent);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else if(aParamType == 17)
	{
		sprintf(cParam,"%d",onlineNumber);
		jstrOnlineStatus=env->NewStringUTF(cParam);
	}
	else
		jstrOnlineStatus=env->NewStringUTF(cParam);

	return jstrOnlineStatus;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_PlayStatusNativeCallback(JNIEnv * env,
		jobject obj,jint aOpenFlag)
{
	env->GetJavaVM(&gJavaVM);
	gPlayStatuJavaObj = env->NewGlobalRef(obj);
//	jclass ClassCJM = env->FindClass("com/example/liverecorddemo/ui/widget/SurFaceViewFactory");
	jclass objclass = env->GetObjectClass(obj);
	if(objclass == 0)
	{
		JPLAYER_LOG_INFO("PlayStatusNativeCallback GetObjectClass failed");
		return;
	}
	gPlayStatusNativeCallback = env->GetMethodID(objclass, "PlayStatusNativeCallback", "(III)V");
	if(gPlayStatusNativeCallback == 0)
	{
		JPLAYER_LOG_INFO("PlayStatusNativeCallback GetMethodID failed");
		return;
	}
//	jstring value = env->NewStringUTF("Hello World!");
//	env->CallVoidMethod(obj, gPlayStatusNativeCallback, 5,10);

	nOpenPlayStatusNativeCallbackFlag = 1;
	return ;
}

JNIEXPORT jstring JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_getDebugInfo(JNIEnv * env,
		jobject obj)
{
	char sDebugInfo[500]={0};
	getDebugInfo(sDebugInfo);
	jstring jstrDebugInfo;
	jstrDebugInfo=env->NewStringUTF(sDebugInfo);
	return jstrDebugInfo;
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_InitSurface(JNIEnv * env,
		jobject obj,
		jobject aSurface,
		jint aWidth,
		jint aHeight)
{
	int ret = 0;

#ifdef USE_HARD_DECODER
#ifdef OMX_DECODE
	ANativeWindow *pNative = ANativeWindow_fromSurface(env,aSurface);

	getVideoManager().setSurface((void *)pNative);
#endif


#ifdef XBMC_DECODE
	if (g_surface != NULL)
	{
		env->DeleteGlobalRef(g_surface);
		g_surface = NULL;
	}
	g_surface = (jobject)env->NewGlobalRef(aSurface);
	getVideoManager().setSurface((void *)g_surface);
#endif

#else

	ret = GetVideoRendererOpenGles20().createRenderer(aWidth, aHeight);

	getVideoManager().signalYUVQueue();

#endif

	JPLAYER_LOG_INFO("GL2JNILib_init,ret:%d width:%d,height:%d\n",
		ret,aWidth,aHeight);

	return ret;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_UninitSurface(JNIEnv * env,
		jobject obj)
{
	getVideoManager().setSurface(NULL);
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_DrawVideo(JNIEnv * env, jobject obj)
{
	getVideoManager().simpleDrawFrame();
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_OpenVideoPlayer(JNIEnv * env,
		jobject obj)
{

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_OpenVideoPlayer \n");

	return openVideoPlayer(-1,-1);

}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_CloseVideoPlayer(JNIEnv * env,
		jobject obj)
{
	closeVideoPlayer();
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_OpenAudioPlayer(JNIEnv * env,
		jobject obj)
{
	return openAudioPlayer();
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_CloseAudioPlayer(JNIEnv * env,
		jobject obj)
{
	closeAudioPlayer();
}


JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_GetPlayPos(JNIEnv * env,
		jobject obj)
{
	return (int)m_pos;
//	return getPlayPos();
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_OpenRecord(JNIEnv * env,
		jobject obj, jstring aFileName)
{
	const char* s = (env)->GetStringUTFChars(aFileName,0);

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_openRecord %s\n",s);

	int nRet = -1;
	if(s != NULL)
	{
		// ....
		nRet = openRecord(s);

		(env)->ReleaseStringUTFChars(aFileName, s);
	}
	return nRet;
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_CloseRecord(JNIEnv * env,
		jobject obj)
{
	closeRecord();
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Snapshot(JNIEnv * env,
		jobject obj, jstring aFileName)
{
	const char* s = (env)->GetStringUTFChars(aFileName,0);

	JPLAYER_LOG_INFO("Java_com_yang_sdk_api_MediaPlayerAPI_Snapshot %s\n",s);

	int nRet = -1;
	if(s != NULL)
	{
		// ....
		nRet = snapshot(s);

		(env)->ReleaseStringUTFChars(aFileName, s);
	}

	return nRet;
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_SetVolume(JNIEnv * env,
		jobject obj, jint aVolume)
{
	return setVolume(aVolume);
}

JNIEXPORT int JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_GetVolume(JNIEnv * env,
		jobject obj)
{
	return getVolume();
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Pause(JNIEnv * env,
		jobject obj)
{
	pausePlay(1);
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_Play(JNIEnv * env,
		jobject obj)
{
	pausePlay(0);
}

JNIEXPORT void JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_SetScale(JNIEnv * env,
		jobject obj, jfloat aScale)
{
	setScale(aScale);
}

JNIEXPORT float JNICALL Java_com_yang_sdk_api_MediaPlayerAPI_GetScale(JNIEnv * env,
		jobject obj)
{
	return getScale();
}



unsigned int Sys_GetTickCount1(void)
{
    unsigned long msecs;
    struct timeval aika;
    gettimeofday(&aika,NULL);
    msecs = aika.tv_sec * 1000;
    msecs += (aika.tv_usec / 1000);
    return msecs;
}

extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	JPLAYER_LOG_INFO("JNI_OnLoad()\n");

	jint version = JNI_VERSION_1_6;
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), version) != JNI_OK)
	{
		return -1;
	}

#ifdef XBMC_DECODE
	int ret = xbmc_jni_on_load(vm, env);
	JPLAYER_LOG_INFO("JNI_OnLoad() ret %d\n", ret);
#endif
	return version;
}



//////////////////////////////////////////////////////////////////
////锟斤拷锟斤拷锟芥�э拷���锟姐�ワ拷锝�锟斤拷���锟�
//////////////////////////////////////////////////////////////////
#ifdef JDEBUG
LONG NETPOSA_CALLBACK g_JPlayerMsgCallBack(LONG  lHandle,
										   LONG  lCommand,
										   LLONG para1,
										   LLONG para2,
										   LONG  dwUser)
{

//	JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand===%ld %ld \n",lCommand,para1);

	if(nOpenPlayStatusNativeCallbackFlag == 1 && firstPlayStatusFlag == 1 && lCommand == JPLAYER_MSG_FIRSTFRAME_TIME)
	{
		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand111===%ld %ld \n",lCommand,para1);
		firstPlayStatusFlag = 0;
		JNIEnv *env;
		gJavaVM->AttachCurrentThread(&env,NULL);
		env->CallVoidMethod(gPlayStatuJavaObj, gPlayStatusNativeCallback,lCommand,para1,para2);
//		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand111 end");
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && lCommand == JPLAYER_MSG_RTMP_RECEIVE_FAILED && playerOpenFlag == 1)
	{
		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand===%ld %ld \n",lCommand,para1);
		JNIEnv *env;
		int result = 0;
		result = gJavaVM->AttachCurrentThread(&env,NULL);
		if(result != 0 )
		{
			JPLAYER_LOG_INFO("g_JPlayerMsgCallBack failed");
		}
		env->CallVoidMethod(gPlayStatuJavaObj, gPlayStatusNativeCallback,lCommand,para1,para2);
//		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand111 end");

		gJavaVM->DetachCurrentThread();
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && (lCommand == JPLAYER_MSG_SESSION_CLOSED || lCommand == JPLAYER_MSG_DATASOURCE_FAILED))
	{
		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand===%ld %ld \n",lCommand,para1);
		JNIEnv *env;
		int result = 0;
		result = gJavaVM->AttachCurrentThread(&env,NULL);
		if(result != 0 )
		{
			JPLAYER_LOG_INFO("g_JPlayerMsgCallBack failed");
		}
		env->CallVoidMethod(gPlayStatuJavaObj, gPlayStatusNativeCallback,lCommand,para1,para2);
//		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack lCommand111 end");

//		gJavaVM->DetachCurrentThread();
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && (lCommand == JPLAYER_MSG_RTMP_UPSTREAM))
	{
		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack rtmpSFrameRate :%d rtmpSBitRate:%d",rtmpSFrameRate,rtmpSBitRate);
		rtmpSFrameRate = para1;
		rtmpSBitRate = para2;
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && (lCommand == JPLAYER_MSG_RTMP_SEND_PERC))
	{
		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack rtmpSendTimePercent:%d onlineNumber:%d",rtmpSendTimePercent,onlineNumber);
		rtmpSendTimePercent = para1;
		onlineNumber = para2;
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && (lCommand == JPLAYER_MSG_BUFFER_LENGTH))
	{
//		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack buffer len:%d Delay time :%d",para1,para2);
		nBufferLen = para1;
		nBufferFrameNum = para2;
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && (lCommand == JPLAYER_MSG_DELAY_TIME))
	{
//		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack Delta time:%d Delay time :%d",para1,para2);
		nDeltaDelayTime = para1;
		nDelayTime = para2;
	}
	else if(nOpenPlayStatusNativeCallbackFlag == 1 && (lCommand == JPLAYER_MSG_DECODE_RESOLUTION))
	{
//		JPLAYER_LOG_INFO("g_JPlayerMsgCallBack JPLAYER_MSG_DECODE_RESOLUTION:%d  :%d",para1,para2);
		if (para1 != 0) {
			nResolutionWidth = (int)para1;
		}

		if (para2 != 0) {
			nResolutionHeight = (int)para2;
		}

	}

	return 0;
}

LONG NETPOSA_CALLBACK g_JPlayerBDCallBack(LONG lHandle,
										   LONG vbps,
										   LONG abps,
										   LONG vfps,
										   LONG afps,
										   LONG svbps,
										   LONG sabps,
										   LONG svfps,
										   LONG safps,
										   LONG dwUser)
{
//	JPLAYER_LOG_INFO("g_JPlayerBDCallBack lHandle:%ld vbps:%ld abps:%ld vfps:%ld afps:%ld\n",
//		lHandle,
//		vbps/1024,
//		abps/1024,
//		vfps,
//		afps);

	fStreamParamVbps = vbps/1024.0;
	fStreamParamAbps = abps/1024.0;
	fStreamParamVfps = (float)vfps;
	fStreamParamAfps = (float)afps;

	aVbpslist[nbpsCount%30] = fStreamParamVbps;
	aAbpslist[nbpsCount%30] = fStreamParamAbps;
	nbpsCount++;

	fStreamParamTotalVbps = 0.0;
	fStreamParamTotalAbps = 0.0;

	for(int i=0;i<30;i++)
	{
		fStreamParamTotalVbps += aVbpslist[i];
		fStreamParamTotalAbps += aAbpslist[i];
	}

//	int ncount = (nbpsCount<30)?nbpsCount:30;

	fStreamParamAVGVbps = fStreamParamTotalVbps/((nbpsCount<30)?nbpsCount:30);
	fStreamParamAVGAbps = fStreamParamTotalAbps/((nbpsCount<30)?nbpsCount:30);

	JPLAYER_LOG_INFO("g_JPlayerBDCallBack :%f %f %f %f %f %f",fStreamParamVbps,fStreamParamAbps,fStreamParamVfps,fStreamParamAfps,fStreamParamAVGVbps,fStreamParamAVGAbps);

#if defined(TOPVDN_IOS_UDPLOG) ||defined(TOPVDN_ANDROID_UDPLOG)
	if(rtmpPlayBeginTime == 0)
		rtmpPlayBeginTime = Sys_GetTickCount1();
	if(Sys_GetTickCount1()-rtmpPlayBeginTime>5*60*1000)
	{
		rtmpPlayBeginTime = Sys_GetTickCount1();
	    ExSendRTMPReceiveLog((unsigned int)fStreamParamAVGVbps,(unsigned int)fStreamParamVfps);
	}
#endif

	if(nOpenStreamInfoNativeCallbackFlag == 1)
	{
		JNIEnv *env;
		gJavaVM->AttachCurrentThread(&env,NULL);
		env->CallVoidMethod(gStreamInfoJavaObj, gStreamInfoNativeCallback,vbps/1024,abps/1024,vfps,afps);
	}

	return 0;
}

static LONG s_lHandle = 0;

//void testSnap()
int  snapshot(const char *p_PicPath)
{
	if (s_lHandle != 0)
	{
//		char path[128];
//		sprintf(path,"/sdcard/%d.bmp",0);
		LONG ok = NET_POSA_JPlayer_Snap(s_lHandle,p_PicPath);
		JPLAYER_LOG_INFO("NET_POSA_JPlayer_Snap,ret:%ld path:%s\n",
			ok,
			p_PicPath);
		return ok;
	}
	return -1;
}

//void testClosePlayer()
void  closeVideoPlayer()
{
	if (s_lHandle != 0)
	{
		NET_POSA_JPlayer_StopSound(s_lHandle);

//		LONG ret = NET_POSA_JPlayer_StopRecord(s_lHandle);
		NET_POSA_JPlayer_Stop(s_lHandle);
//		JPLAYER_LOG_INFO("NET_POSA_JPlayer_Stop===%ld\n",ret);

		JPLAYER_LOG_INFO("NET_POSA_JPlayer_Stop ");
		NET_POSA_JPlayer_Close(s_lHandle);
		s_lHandle = 0;
		JPLAYER_LOG_INFO("NET_POSA_JPlayer_Close over");
	}

}

//void testPlayer()
int  openVideoPlayer(int p_MinMediaBufferLength, int p_MaxMediaBufferLength)
{
	rtmpPlayBeginTime = 0;
	m_pos = 0;
	fStreamParamVbps = 0.0;
	fStreamParamAbps = 0.0;
	fStreamParamVfps = 0.0;
	fStreamParamAfps = 0.0;
	fStreamParamAVGVbps = 0.0;
	fStreamParamAVGAbps = 0.0;
	fStreamParamTotalVbps = 0.0;
	fStreamParamTotalAbps = 0.0;
	memset(aVbpslist,0,120);
	memset(aAbpslist,0,120);
	nbpsCount = 0;
	rtmpSFrameRate = 0;
	rtmpSBitRate = 0;
	rtmpSendTimePercent = 0;
	onlineNumber = 0;

	streamMode = getStreamMode();

	if (s_lHandle != 0)
	{
		JPLAYER_LOG_INFO("openVideoPlayer  s_lHandle != 0");
		return -1;
	}

//	if(streamMode == 2 /*&& (request_list == NULL || fromtime == totime)*/)
//	{
//		JPLAYER_LOG_INFO("recordlist param error");
//		return -1;
//	}

	JPLAYER_LOG_INFO("openVideoPlayer  s_lHandle != 0");
	static int s_initone = 0;
	if (s_initone == 0)
	{
		NET_POSA_JPlayer_Init();
		s_initone = 1;
	}

	NET_POSA_JPlayer_SetMsgCallBack(g_JPlayerMsgCallBack,(LONG)0);

	JPlayer_Info Info;
	LONG ok = NET_POSA_JPlayer_GetLibraryInfo(&Info);
	if (ok == JPLAYER_MSG_NORMAL)
	{
		JPLAYER_LOG_INFO("%s\n",Info.m_version);
	}

	JPlayer_PlaySession sess;  //  ip session
	s_lHandle = NET_POSA_JPlayer_Open(&sess) ;
	JPLAYER_LOG_INFO("JPlayer_Open===%ld\n",s_lHandle);
	if (s_lHandle != 0)
	{
		firstPlayStatusFlag = 1;

		NET_POSA_JPlayer_SetBDCallBack(s_lHandle,1,g_JPlayerBDCallBack,0);

		if(streamMode == 2)
		{
			JPLAYER_LOG_INFO("from :%d  to:%d ",fromtime,totime);
			NET_POSA_JPlayer_SetRecordPlaySource(s_lHandle,fromtime,totime);
		}

		LONG ret = NET_POSA_JPlayer_Play_Live(s_lHandle,streamMode, p_MinMediaBufferLength, p_MaxMediaBufferLength);
		JPLAYER_LOG_INFO("JPlayer_Play_Live===%ld playMode:%d streamMode:%d \n",ret,streamMode);
		if (ret != JPLAYER_MSG_NORMAL)
		{
			NET_POSA_JPlayer_Close(s_lHandle);
			s_lHandle = 0;
			return -1;
		}
		else
		{
			NET_POSA_JPlayer_PlaySound(s_lHandle);
		}
//		openAudioPlayer();
	}

	return 0;
}

int locateStream(long p_CurrentTime)
{
	return NET_POSA_JPlayer_LocateStream(s_lHandle,p_CurrentTime);
}

int  openAudioPlayer()
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_PlaySound(s_lHandle);
	}
}

void  closeAudioPlayer()
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_StopSound(s_lHandle);
	}
}

int   setVolume(int p_nVolume)
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_SetVolume(p_nVolume);
	}
}

int   getVolume()
{
	if (s_lHandle != 0)
	{
		unsigned int pVolume;
		LONG ret = NET_POSA_JPlayer_GetVolume(&pVolume);
	}
}

void  pausePlay(const int p_PauseFlag)
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_Pause(s_lHandle, p_PauseFlag);
	}
}

void  setPlayPos(long p_lRelativePos)
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_SetPlayPos(s_lHandle, p_lRelativePos);
	}
}

int  getPlayPos()
{
	if (s_lHandle != 0)
	{
		int ret = NET_POSA_JPlayer_GetPlayPos(s_lHandle);
		return ret;
	}
	return 0;
}

void  setScale(float p_fScale)
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_SetScale(s_lHandle, p_fScale);
		return ;
	}
}

float  getScale()
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_GetScale(s_lHandle);
		return ret;
	}
}

int  openRecord(const char * p_nFileName)
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_StartRecord(s_lHandle, 1|2, p_nFileName);
		return ret;
	}
}

void  closeRecord()
{
	if (s_lHandle != 0)
	{
		LONG ret = NET_POSA_JPlayer_StopRecord(s_lHandle);
//		return ret;
	}
}
//
//int openAudioCapture(const char * p_SStreamPath)
//{
//	return NET_POSA_JPlayer_OpenAudioCapture(s_lHandle,p_SStreamPath);
//}

//int  ConnectPublicLive(const char *  aCameraHashID)
//{
//	streamMode = 3;
//
//	char * url = new char [URL_LENTH*2];
//	memset(url,0,URL_LENTH*2);
//
//	sprintf(url,"http://120.26.65.144:8005/API/rtmp/%s/addr", aCameraHashID);//shenzhen
//	//	sprintf(url,"http://115.29.186.62:8005/API/cameras/%d/playdata?from=%d&to=%d", nCid, p_From,p_To);//hangzhou
//
//	JPLAYER_LOG_INFO("httpGet rtmp :%s",url);
//
//	char *response = NULL;
//	int responselen = 0;
//	int nRet  = httpGet(url,&response,&responselen,20);
//
//	if(nRet == 0)
//		JPLAYER_LOG_INFO("httpGet  sucess :%s ",response);
//	else
//	{
//		JPLAYER_LOG_INFO("httpGet  failed :%d ",nRet );
//		char errorlog[50];
//		sprintf(errorlog,"ConnectPublicLive get rtmpurl failed:%d",nRet);
//		sendLog(errorlog);
//		return -1;
//	}
//
//	Json::Reader reader;
//	Json::Value root;
//	if (reader.parse(response, root))
//	{
//		std::string srtmp = root["data"]["rtmp_addr"].asString();
//		JPLAYER_LOG_INFO("rtmp_addr :%s ",srtmp.c_str());
//
////		int findIP = str_sunday_search(srtmp.c_str()+7,20,":",1);
////		if(findIP > 0)
////		{
////			memcpy(m_RTMPURL,srtmp.c_str()+7,findIP);
////			JPLAYER_LOG_INFO("IP:%s",m_RTMPURL);
////		}
//
////		char cid[8];
////		convertStringtoBinary(aCameraHashID,cid);
////		JPLAYER_LOG_INFO("hashid :%s",cid);
//		sendRtmpUrlLog(1, aCameraHashID, strlen(aCameraHashID));
//
//		sprintf(streampath,"%s",srtmp.c_str());
////		sprintf(streampath,"rtmp://192.168.0.107/demo/666666");
//		memset(m_RTMPURL,0,20);
//		nRet = openVideoPlayer();
////		JPLAYER_LOG_INFO("rtmp url :%s",m_RTMPURL);
//		return nRet;
//	}
//	return -1;
//	//smooth
//}

static int translate_message(const char* message, unsigned char* buf, size_t size)
{
    size_t i;
    unsigned int b;
    if (strlen(message) < (size<<1)) {
        // message长度必须是size的2倍以上
        return -1;
    }
    for (i=0; i<size; i++, message+=2) {
        sscanf(message, "%02X", &b);

        buf[i] = (unsigned char)(b&0xFF);

//        JPLAYER_LOG_INFO("translate_message :%02x  %02x",b,buf[i]);
    }
    return 0;
}

static void* CloudMessageHandler(void* p)
{
	char* msg;
	JNIEnv *env;
	jstring smessage;

	gJavaVM->AttachCurrentThread(&env, NULL);

	while (bCloudMessageProcessing) {
		msg = NULL;
		if (queCloudMessage.pop(msg) && msg) {
			smessage = env->NewStringUTF(msg);
			if (smessage) {
//				JPLAYER_LOG_INFO("Handle message: %s", msg);
				env->CallVoidMethod(gMessageJavaObj, gMessageNativeCallback, smessage);
				env->DeleteLocalRef(smessage);
			}
			free(msg);
		}
	}

	return NULL;
}

static void StopCloudMessageProcessing()
{
	if (bCloudMessageProcessing) {
		bCloudMessageProcessing = 0;
		queCloudMessage.push(NULL);
		pthread_join(tidCloudMessage, NULL);
	}
}

int PopMessage(void *apData, const char *aMessage)
{
	cJSON *root = cJSON_Parse(aMessage);
	if (root == NULL) {
		return -1;
	}
	cJSON *name = cJSON_GetObjectItem(root, "Name");
	if (name == NULL) {
		return -1;
	}
	JPLAYER_LOG_INFO("popmessage :%s ",name->valuestring);
	if (memcmp("ConnectionAcceptted", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("＝＝＝＝＝＝＝ConnectionAcceptted＝＝＝＝＝＝＝＝");
//		if(gMessageNativeCallback != 0)
//		{
//			JNIEnv *env;
//			gJavaVM->AttachCurrentThread(&env,NULL);
//			jstring smessage;
////			smessage=env->NewStringUTF("ConnectionAcceptted");
//			smessage=env->NewStringUTF(aMessage);
//			env->CallVoidMethod(gMessageJavaObj, gMessageNativeCallback,smessage);
//			env->DeleteLocalRef(smessage);
//		}

		char* newMsg = NULL;

		cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
		if (jcid) {
			char hash[32] = {0};
			get_hashid_from((unsigned int)jcid->valueint, hash);
			cJSON* jnewCid = cJSON_CreateString(hash);
			if (jnewCid) {
				cJSON_ReplaceItemInObject(root, "SrcID", jnewCid);
			}
			newMsg = cJSON_PrintUnformatted(root);
		} else {
			newMsg = strdup(aMessage);
		}

//		cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
//		if (jcid) {
//			JsonSubtitute jsonsub;
//			if (jsonsub.ParseJson(aMessage)) {
//				char hash[32] = {0};
//				get_hashid_from((unsigned int)jcid->valueint, hash);
//				if (jsonsub.Subtitute("SrcID", hash)) {
//					newMsg = jsonsub.JsonDup();
//				} else {
//					JPLAYER_LOG_ERROR("Subtitute 'PopMessage:SrcID' failed");
//				}
//			}
//		} else {
//			newMsg = strdup(aMessage);
//		}
		if (newMsg && bCloudMessageProcessing) {
			queCloudMessage.push(newMsg);
		}

		*(int *)apData = 1;
	}
	else if (memcmp("ConnectionClosed", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("ConnectionClosed");
		if(privatePlayingFlag == 1)
		{
//			if(gMessageNativeCallback != 0)
//			{
//				JNIEnv *env;
//				gJavaVM->AttachCurrentThread(&env,NULL);
//				jstring smessage;
////				smessage=env->NewStringUTF("ConnectionClosed");
//				smessage=env->NewStringUTF(aMessage);
//				env->CallVoidMethod(gMessageJavaObj, gMessageNativeCallback,smessage);
//				env->DeleteLocalRef(smessage);
//			}

			char* newMsg = NULL;

			cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
			if (jcid) {
				char hash[32] = {0};
				get_hashid_from((unsigned int)jcid->valueint, hash);
				cJSON* jnewCid = cJSON_CreateString(hash);
				if (jnewCid) {
					cJSON_ReplaceItemInObject(root, "SrcID", jnewCid);
				}
				newMsg = cJSON_PrintUnformatted(root);
			} else {
				newMsg = strdup(aMessage);
			}

//			cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
//			if (jcid) {
//				JsonSubtitute jsonsub;
//				if (jsonsub.ParseJson(aMessage)) {
//					char hash[32] = {0};
//					get_hashid_from((unsigned int)jcid->valueint, hash);
//					if (jsonsub.Subtitute("SrcID", hash)) {
//						newMsg = jsonsub.JsonDup();
//					} else {
//						JPLAYER_LOG_ERROR("Subtitute 'PopMessage:SrcID' failed");
//					}
//				}
//			} else {
//				newMsg = strdup(aMessage);
//			}
			if (newMsg && bCloudMessageProcessing) {
				queCloudMessage.push(newMsg);
			}

			*(int *)apData = 2;
		}
	}
	else if (memcmp("Online", name->valuestring, strlen(name->valuestring)) == 0)
	{
		char* newMsg = NULL;
		newMsg = strdup(aMessage);
		if (newMsg && bCloudMessageProcessing) {
			queCloudMessage.push(newMsg);
		}
	}
	else if (memcmp("Offline", name->valuestring, strlen(name->valuestring)) == 0)
	{
		char* newMsg = NULL;
		newMsg = strdup(aMessage);
		if (newMsg && bCloudMessageProcessing) {
			queCloudMessage.push(newMsg);
		}
	}
	else if (memcmp("PopMessage", name->valuestring, strlen(name->valuestring)) == 0)
	{
		cJSON *messgae = cJSON_GetObjectItem(root, "Message");
		JPLAYER_LOG_INFO("PopMessage :%s ",messgae->valuestring);
//		if(gMessageNativeCallback != 0)
		{
			unsigned char buffer[260]={0};
			int nRet = translate_message(messgae->valuestring,buffer,strlen(messgae->valuestring)/2);
//			JPLAYER_LOG_INFO("  %.2x %.2x %.2x %.2x %.2x %.2x PopMessage :%s translate_message:%d",
//					buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer,nRet);
//			JNIEnv *env;
//			gJavaVM->AttachCurrentThread(&env,NULL);
//			jstring smessage;
////			smessage=env->NewStringUTF(buffer);
//			smessage=env->NewStringUTF(aMessage);
//			env->CallVoidMethod(gMessageJavaObj, gMessageNativeCallback,smessage);
//			env->DeleteLocalRef(smessage);

			char* newMsg = NULL;

			cJSON* jnewMessage = cJSON_CreateString((char*)buffer);
			if (jnewMessage) {
				cJSON_ReplaceItemInObject(root, "Message", jnewMessage);
				cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
				if (jcid) {
					char hash[32] = {0};
					get_hashid_from((unsigned int)jcid->valueint, hash);
					cJSON* jnewCid = cJSON_CreateString(hash);
					if (jnewCid) {
						cJSON_ReplaceItemInObject(root, "SrcID", jnewCid);
					}
				}
				newMsg = cJSON_PrintUnformatted(root);
			} else {
				newMsg = strdup(aMessage);
			}
//			JPLAYER_LOG_INFO("cJSON_PrintUnformatted: %s", newMsg);

//			Json::Reader jreader;
//			Json::Value jroot;
//			if (jreader.parse(aMessage, jroot)) {
//				jroot["Message"] = (const char*)buffer;
//				newMsg = strdup(jroot.toStyledString().c_str());
//			}

//			JsonSubtitute jsonsub;
//			if (jsonsub.ParseJson(aMessage)) {
//				if (jsonsub.Subtitute("Message", (char*)buffer)) {
//					cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
//					if (jcid) {
//						char hash[32] = {0};
//						get_hashid_from((unsigned int)jcid->valueint, hash);
//						jsonsub.Subtitute("SrcID", hash);
//						JPLAYER_LOG_INFO("Subtitute SrcID %p=%s", hash, hash);
//					}
//					newMsg = jsonsub.JsonDup();
//				} else {
//					JPLAYER_LOG_ERROR("Subtitute 'PopMessage:Message' failed");
//				}
//			} else {
//				newMsg = strdup(aMessage);
//			}
			if (newMsg && bCloudMessageProcessing) {
				queCloudMessage.push(newMsg);
			}
		}

	}
	else if (memcmp("NewBoundCamera", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("NewBoundCamera");
	}
	else if (memcmp("UnBoundCamera", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("UnBoundCamera");
	}
	else if (memcmp("NewAuthCamera", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("NewAuthCamera");
	}
	else if (memcmp("CancelAuthCamera", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("CancelAuthCamera");
	}
	else if (memcmp("RtmpConnect", name->valuestring, strlen(name->valuestring)) == 0)
	{
	//当配置了rtmp服务之后，会收到开启rtmp服务的RtmpConnect连接请求
//		int ret = 0;
//		if(publicLiveFlag == 1)
//			ret = D_SetRtmpConnectMessage(messgae->valuestring,Cfg_State,Cfg_ScaleID);
//		else
//			ret = D_SetRtmpConnectMessage(messgae->valuestring,Cfg_State,Cfg_AreaID);
//		if(ret > 0)
//		{
//			rtmp_connect_flag =1;
//			openCapture();
//		}
//		else if(ret < 0)
//			printf("error , change config or do restart service!\n");
	}
	else if (memcmp("PopConfigResult", name->valuestring, strlen(name->valuestring)) == 0)
	{
		JPLAYER_LOG_INFO("PopConfigResult");
//		if(gMessageNativeCallback != 0)
		{
//			JNIEnv *env;
//			gJavaVM->AttachCurrentThread(&env,NULL);
//			jstring smessage;
////			smessage=env->NewStringUTF("ConnectionAcceptted");
//			smessage=env->NewStringUTF("{\"Name\":	\"PopMessage\",\"Message\":	\"PopConfigResult\",\"SrcID\":	1003190}");
//			env->CallVoidMethod(gMessageJavaObj, gMessageNativeCallback,smessage);
//			env->DeleteLocalRef(smessage);

			char* newMsg = NULL;
			cJSON* jcid = cJSON_GetObjectItem(root, "SrcID");
			if (jcid) {
				char hash[32] = {0};
				get_hashid_from((unsigned int)jcid->valueint, hash);
				cJSON* jnewCid = cJSON_CreateString(hash);
				if (jnewCid) {
					cJSON_ReplaceItemInObject(root, "SrcID", jnewCid);
				}
				newMsg = cJSON_PrintUnformatted(root);
//				JsonSubtitute jsonsub;
//				if (jsonsub.ParseJson(aMessage)) {
//					char hash[32] = {0};
//					get_hashid_from((unsigned int)jcid->valueint, hash);
//					if (jsonsub.Subtitute("SrcID", hash)) {
//						newMsg = jsonsub.JsonDup();
//					} else {
//						JPLAYER_LOG_ERROR("Subtitute 'PopMessage:SrcID' failed");
//					}
//				}
			} else {
				newMsg = strdup(aMessage);
			}
			if (newMsg && bCloudMessageProcessing) {
				queCloudMessage.push(newMsg);
			}
		}
	}
	else if (!strcmp(name->valuestring, "DeviceBindConfirm")) {
		char* newMsg = NULL;
		newMsg = strdup(aMessage);
		if (newMsg && bCloudMessageProcessing) {
			queCloudMessage.push(newMsg);
		}
	}
	cJSON_Delete(root);
	return 0;
}

int messageReceiveFlag = 0;
pthread_t messageReceive;

void messageReceiveThread()
{
	while(messageReceiveFlag)
	{
		if(messageType == 1)
		{
			messageType = 0;
			openVideoPlayer(-1,-1);
			JPLAYER_LOG_INFO("openVideoPlayer");
		}
		if(messageType == 2)
		{
			messageType = 0;
			closeVideoPlayer();
			JPLAYER_LOG_INFO("closeVideoPlayer");


			JNIEnv *env;
			gJavaVM->AttachCurrentThread(&env,NULL);
			env->CallVoidMethod(gPlayStatuJavaObj, gPlayStatusNativeCallback,JPLAYER_MSG_UDP_DISCONNECT,0,0);

		}
		usleep(20*1000);
	}
}

int appStartCloudService(const char* aAppID, const char* aUsername, const char* aPassword, const char* aConfigString)
{
	int nRet = ExStartCloudService(aAppID,aUsername,aPassword,aConfigString, &messageType,PopMessage);
	if(nRet != 0)
		return nRet;

	if(messageReceiveFlag == 0)
	{
		messageReceiveFlag = 1;
		int ret = pthread_create(&messageReceive, NULL, (void *)messageReceiveThread, NULL);
		JPLAYER_LOG_INFO("pthread_create message receive thread :%d",ret);
		if(ret != 0)
		{
			usleep(4*1000*1000);
			JPLAYER_LOG_INFO("pthread_create message receive thread failed");
//			char messagelog[50]={0};
//			sprintf(messagelog,"pthread_create message receive thread failed :%d",ret);
//			SendLog(messagelog);
		}
	}
	return nRet;
}

int appStartCloudServiceV2(const char* aUserToken, const char* aConfigString)
{
	int nRet = StartCloudServiceV2(aUserToken,aConfigString, &messageType,PopMessage);
	if(nRet != 0)
		return nRet;

	if(messageReceiveFlag == 0)
	{
		messageReceiveFlag = 1;
		int ret = pthread_create(&messageReceive, NULL, (void *)messageReceiveThread, NULL);
		JPLAYER_LOG_INFO("pthread_create message receive thread :%d",ret);
		if(ret != 0)
		{
			usleep(4*1000*1000);
			JPLAYER_LOG_INFO("pthread_create message receive thread failed");
//			char messagelog[50]={0};
//			sprintf(messagelog,"pthread_create message receive thread failed :%d",ret);
//			SendLog(messagelog);
		}
	}
	return nRet;
}

void appStopCloudService()
{
	if(messageReceiveFlag == 1)
	{
		messageReceiveFlag = 0;
		pthread_join(messageReceive, NULL);
	}
	StopCloudService();
}

void audio_consume(char* audio_data,int len)
{
	if (s_lHandle != 0)
	{
		NET_POSA_JPlayer_playAudio(s_lHandle,audio_data,len);
	}
}

#endif










