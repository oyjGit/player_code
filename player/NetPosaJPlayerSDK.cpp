#ifdef WIN32
#include "npdebug.h"
#include <Windows.h>
#endif

#include "PlayerManager.h"
#include "NetPosaJPlayerSDK.h"
#include "TSingleton.h"

#define SOFT_VERSION "2.0.0.1"


typedef comn::Singleton<PlayerManager> SingletonPlayerManager;

PlayerManager& GetPlayerManager()
{
	return SingletonPlayerManager::instance();
}


#ifdef __cplusplus
extern "C"
{
#endif


LONG NETPOSA_METHOD NET_POSA_JPlayer_Init()
{
	return PlayerManager::init()?JPLAYER_MSG_NORMAL:JPLAYER_MSG_NO_INIT_FAIL;
}


LONG NETPOSA_METHOD NET_POSA_JPlayer_Cleanup()
{
	return PlayerManager::unInit()?JPLAYER_MSG_NORMAL:JPLAYER_MSG_NO_INIT_FAIL;
}

/// ��ȡ��������������Ϣ.
/// @param pInfo   ���������Ϣ
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_GetLibraryInfo(JPlayer_Info* pInfo)
{
	if (pInfo == NULL)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	strcpy(pInfo->m_desc,"jplayer");
	strcpy(pInfo->m_version,SOFT_VERSION);
	return JPLAYER_MSG_NORMAL;
}

/// ��ȡ��ǰ�Ự����Ĵ�����.
/// @param[in] lHandle  �Ự���
/// @return  ������
LONG NETPOSA_METHOD NET_POSA_JPlayer_GetLastError(LONG lHandle)
{
	return GetPlayerManager().JPlayer_GetLastError(lHandle);
}


/// ���úͷ������ĻỰ��Ϣ�ص�.
/// @param[in] jpMsgCallback ��Ϣ�ص�
/// @param[in] dwUser   �û����
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_SetMsgCallBack(
												JPlayerMsgCallBack jpMsgCallback,
												LONG dwUser)
{
	return GetPlayerManager().JPlayer_SetMsgCallBack(jpMsgCallback,dwUser);
}




/// �򿪺ͷ������ĻỰ.
/// @param sess  �Ự����
/// @return      �Ự��� �ɹ� > 0��ʧ�� < 0�����룩��
LONG NETPOSA_METHOD NET_POSA_JPlayer_Open(JPlayer_PlaySession *pSess) 
{
	if (pSess == NULL)
	{
		return JPLAYER_MSG_NO_INIT_FAIL; 
	}
	return GetPlayerManager().JPlayer_Open(*pSess);
}

LONG NETPOSA_METHOD NET_POSA_JPlayer_SetPlaySession(LONG lHandle,
												JPlayer_PlaySession *pSess)
{
	if (pSess == NULL)
	{
		return JPLAYER_MSG_NO_INIT_FAIL; 
	}
	return GetPlayerManager().JPlayer_SetPlaySession(lHandle,*pSess);
}


/// �رպͷ������ĻỰ.
/// @param[in] lHandle �Ự���
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_Close(LONG lHandle)
{
	return GetPlayerManager().JPlayer_Close(lHandle);
}

/// ¼���ѯ.
/// @param req  ��ѯ��������
/// @param resp ��ѯ���
/// @return     ������
LONG NETPOSA_METHOD NET_POSA_JPlayer_QueryRecord(LONG lHandle,
									    JPlayer_QueryRecordRequest req, 
										JPlayer_QueryRecordResponse* resp) 
{
	if (resp == NULL)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}
	return GetPlayerManager().JPlayer_QueryRecord(lHandle,req,*resp);
}


/// ��ʼ����ʵʱ��Ƶ.
/// @param[in] lHandle �Ự���
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_Play_Live(LONG lHandle,int p_StreamMode, int p_MinMediaBufferLength, int p_MaxMediaBufferLength)
{
	return GetPlayerManager().JPlayer_Play_Live(lHandle,p_StreamMode, p_MinMediaBufferLength, p_MaxMediaBufferLength);
}


// ��ʼ������ʷ��Ƶ.
/// @param[in] lHandle �Ự���
/// @param[in] item    ��ʷ��Ƶ��Ϣ
/// @return 	
LONG NETPOSA_METHOD NET_POSA_JPlayer_Play_Record(LONG lHandle, 
												JPlayer_RecordItem item)
{
	
	return GetPlayerManager().JPlayer_Play_Record(lHandle,item);
}

/// ֹͣ����.
/// @param[in] lHandle �Ự���
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_Stop(LONG lHandle)
{
	return GetPlayerManager().JPlayer_Stop(lHandle);
}


/// ��ʼ������Ƶ.
/// @param[in] lHandle �Ự���
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_PlaySound(LONG lHandle)
{
	return GetPlayerManager().JPlayer_PlaySound(lHandle);
}

/// ֹͣ������Ƶ.
/// @param[in] lHandle �Ự���
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_StopSound(LONG lHandle)
{
	return GetPlayerManager().JPlayer_StopSound(lHandle);
}


/// ���ò��ŵ����λ��.
/// @param[in] lHandle �Ự���
/// @param[in] lRelativePos ����λ�ã�
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_SetPlayPos(LONG lHandle, LLONG lRelativePos)
{
	return GetPlayerManager().JPlayer_SetPlayPos(lHandle,lRelativePos);
}

/// ��ò��ŵ����λ��.
/// @param[in] lHandle �Ự���
/// @return ����λ��,
LLONG NETPOSA_METHOD NET_POSA_JPlayer_GetPlayPos(LONG lHandle)
{
	return GetPlayerManager().JPlayer_GetPlayPos(lHandle);
}



/// ��ͣ/�ָ�����.
/// @param[in] lHandle �Ự���
/// @param[in] nPause ��ʶ��1-��ͣ ����ֵ��ʾ�ָ�����
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_Pause(LONG lHandle,unsigned int nPause)
{
	return GetPlayerManager().JPlayer_Pause(lHandle,nPause);
}



/// ���ñ��ٲ���.
/// @param[in] lHandle �Ự���
/// @param[in] scale   �����ٶ� ��ȡֵ��Χ ��1/8 --- 8.0��
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_SetScale(LONG lHandle,float scale)
{
	return GetPlayerManager().JPlayer_SetScale(lHandle,scale);
}


/// ��ȡ���ٲ���
/// @param[in] lHandle ��ž��
/// @return �����ٶ� ��ȡֵ��Χ ��1/8 --- 8.0��
float NETPOSA_METHOD NET_POSA_JPlayer_GetScale(LONG lHandle)
{
	return GetPlayerManager().JPlayer_GetScale(lHandle);
}




/// ������Ƶ����
/// @param  volume ��������ΧΪ��[0-100], 0 ��ʾ����
/// @return TRUE   ��ʾ�ɹ�

LONG NETPOSA_METHOD NET_POSA_JPlayer_SetVolume(unsigned int  volume)
{
	return GetPlayerManager().JPlayer_SetVolume(volume);
}


/// ��ȡ����
/// @param  pVolume ����
/// @return TRUE    ��ʾ�ɹ�
LONG NETPOSA_METHOD NET_POSA_JPlayer_GetVolume(unsigned int*  pVolume)
{
	return GetPlayerManager().JPlayer_GetVolume(pVolume);
}


/// ץͼ.
/// @param[in] lHandle �Ự���
/// @param[in] path    �ļ�·��
/// @return TRUE-�ɹ���FALSE-ʧ��
LONG NETPOSA_METHOD NET_POSA_JPlayer_Snap(LONG lHandle,const char *path)
{
	return GetPlayerManager().JPlayer_Snap(lHandle,path);
}


LONG NETPOSA_METHOD NET_POSA_JPlayer_SendStream(LONG lHandle,
												LLONG          serialNum, 
												unsigned int   frameType, 
												int            isKey, 
												unsigned char* pFrame, 
												unsigned int   frameLength, 
												LLONG          timestamp)
{
	return GetPlayerManager().JPlayer_SendStream( lHandle,
		 serialNum, 
		  frameType, 
		  isKey, 
		 pFrame, 
		  frameLength, 
		 timestamp);
}


LONG NETPOSA_METHOD NET_POSA_JPlayer_SetBDCallBack(
											LONG lHandle,
											unsigned int nSecond,
											JPlayerBDCallBack jpBDCallback,
											LONG dwUser)
{
	return GetPlayerManager().JPlayer_SetBDCallBack( lHandle,
		nSecond,
		jpBDCallback, 
		dwUser);

}

LONG NETPOSA_METHOD NET_POSA_JPlayer_SetAudioEncode(LONG lHandle,
													int sampleRate,
													int channel,
													int fps)
{
	return GetPlayerManager().JPlayer_SetAudioEncode( lHandle,
		sampleRate,
		channel, 
		fps);
}

/// ������Ƶ�������
/// @param[in] lHandle �Ự���
/// @param[in] sampleRate ������
/// @param[in] channel    ͨ������
/// @return ������
LONG NETPOSA_METHOD NET_POSA_JPlayer_SetAudioDecode(LONG lHandle,
													int sampleRate,
													int channel)
																 
{
	return GetPlayerManager().JPlayer_SetAudioDecode( lHandle,
		sampleRate,
		channel);
}

/// ������Ϣ��relay
/// @param[in] lHandle �Ự���
/// @param[in] pMsg    ��Ϣ����
/// @return    ������
LONG NETPOSA_METHOD NET_POSA_JPlayer_SendMessage(LONG lHandle,
												 const char *pMsg)
{
	if (pMsg == NULL)
	{
		return JPLAYER_MSG_NO_INIT_FAIL;
	}

	return GetPlayerManager().JPlayer_SendMessage(lHandle,pMsg);
}


/// ��ʼ¼��.
/// @param[in] lHandle �Ự���
/// @param[in] track  @see JPlayer_MP4_RECORD 
///                    JPLAYER_MP4_VIDEO_TRACKֻ¼��Ƶ
///                    JPLAYER_MP4_AUDIO_TRACKֻ¼��Ƶ
///                    JPLAYER_MP4_VIDEO_TRACK | JPLAYER_MP4_VIDEO_TRACK ¼����Ƶ
/// @param[in] path   ·����Ŀǰֻ֧��MP4������������Ƶ��ʽΪH264����ƵΪС��PCM16
/// @return �����룬ע��ͬʱֻ��¼��һ���ļ���¼������ʧ�ܿ��ܻ�ͨ����Ϣ�ص�֪ͨJPLAYER_MSG_RECORD_ERROR����
LONG NETPOSA_METHOD NET_POSA_JPlayer_StartRecord(LONG lHandle,
												int track,
												const char *path)
{
	return GetPlayerManager().JPlayer_StartRecord(lHandle,track,path);
}


/// ֹͣ¼��.
/// @param[in] lHandle �Ự���
/// @return ������
LONG NETPOSA_METHOD NET_POSA_JPlayer_StopRecord(LONG lHandle)
{
	return GetPlayerManager().JPlayer_StopRecord(lHandle);
}

NET_POSA_API LONG NETPOSA_METHOD NET_POSA_JPlayer_LocateStream(LONG lHandle, long p_CurrentTime)
{
	return GetPlayerManager().JPlayer_LocateStream(lHandle,p_CurrentTime);
}
//
//NET_POSA_API LONG NETPOSA_METHOD NET_POSA_JPlayer_OpenAudioCapture(LONG lHandle, const char *p_StreamPath)
//{
//	return GetPlayerManager().JPlayer_OpenAudioCapture(lHandle,p_StreamPath);
//}

NET_POSA_API LONG NETPOSA_METHOD NET_POSA_JPlayer_SetRecordPlaySource(LONG lHandle,long from,long to)
{
	return GetPlayerManager().JPlayer_SetRecordPlaySource(lHandle,from,to);
}

NET_POSA_API void NETPOSA_METHOD NET_POSA_JPlayer_playAudio(LONG lHandle, char * data, int datalen)
{
	GetPlayerManager().playAudio(lHandle,data,datalen);
}

#ifdef __cplusplus
}
#endif






