#include "stdafx.h"
#include "AudioSounder.h"
#include "GlobalVariable.h"
#include "LogOutput.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p) = NULL; } }
#endif 

CAudioSounder::CAudioSounder(void)
	: mDsbPosNotify(NULL)
	, mDsbPosEvent(NULL)
	, mDirectSoundBuf8(NULL)
	, mDirectSoundBuffer(NULL)
	, mDirectSound(NULL)
	, mInitialized(false)
	, mNotifyTimeout(0)

{
	mDsbPosNotify = new DSBPOSITIONNOTIFY[CGlobalVariable::instance().gDsoundBufNotifyCount];
	mDsbPosEvent = new HANDLE[CGlobalVariable::instance().gDsoundBufNotifyCount];

	for (int i = 0; i < CGlobalVariable::instance().gDsoundBufNotifyCount; i++)
	{
		mDsbPosNotify[i].dwOffset = 0;
		mDsbPosNotify[i].hEventNotify = NULL;
		mDsbPosEvent[i] = NULL;
	}
	mVolume = 0;
	memset(&mBufferDesc, 0, sizeof(DSBUFFERDESC));
	memset(&mWaveFormat, 0, sizeof(WAVEFORMATEX));
}


CAudioSounder::~CAudioSounder(void)
{
	if (NULL != mDsbPosNotify)
	{
		delete []mDsbPosNotify;
		mDsbPosNotify = NULL;
	}
	if (NULL != mDsbPosEvent)
	{
		delete []mDsbPosEvent;
		mDsbPosEvent = NULL;
	}
}

/**
 *@name       init
 *@brief      ��ʼ��������������
 *@param[in]  WndHandle ���ڽ����¼��Ĵ��ھ����
 *@param[in]  channels pcm��ͨ������
 *@param[in]  SampleBits pcm�Ĳ���λ����
 *@param[in]  SampeRate pcm�Ĳ����ʡ�
 *@return     true��ʾ�ɹ���false��ʾʧ�ܡ�
 */
bool CAudioSounder::init(HWND WndHandle, long channels, long SampleBits, long SampeRate)
{
	mInitialized = false;

	mWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	mWaveFormat.nChannels = (WORD)channels;
	mWaveFormat.nSamplesPerSec = SampeRate;
	mWaveFormat.wBitsPerSample = (WORD)SampleBits;
	mWaveFormat.nBlockAlign = (mWaveFormat.wBitsPerSample / 8) * mWaveFormat.nChannels;
	mWaveFormat.nAvgBytesPerSec = mWaveFormat.nSamplesPerSec * mWaveFormat.nBlockAlign;

	mDirectSound = NULL;
	if(FAILED(DirectSoundCreate8(NULL, &mDirectSound, NULL)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ�����DirectSound����");

	//����Э������.DSSCL_PRIORITY:�����ռ;DSSCL_NORMAL:������������
	if(FAILED(mDirectSound->SetCooperativeLevel(WndHandle, DSSCL_PRIORITY)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ�����DirectSoundЭ���ȼ�");

	DSCAPS dscaps;

	dscaps.dwSize = sizeof(DSCAPS);
	if (FAILED(mDirectSound->GetCaps(&dscaps)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ���ȡ�豸����");

	mBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	mBufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2| DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE;
	mBufferDesc.dwBufferBytes = CGlobalVariable::instance().gDsoundBufNotifyCount * CGlobalVariable::instance().gDsoundBufNotifySize;
	mBufferDesc.lpwfxFormat = &mWaveFormat;

	// mDirectSoundBufferΪ�λ������Ľӿ�ָ��,mark������������
	mDirectSoundBuffer = NULL;
	if(FAILED(mDirectSound->CreateSoundBuffer(&mBufferDesc, &mDirectSoundBuffer, NULL)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ�����DirectSound������. NotifySize = %d, NotifyCount = %d", CGlobalVariable::instance().gDsoundBufNotifySize, CGlobalVariable::instance().gDsoundBufNotifyCount);

	// mDirectSoundBuf8Ϊ���°汾�λ������Ľӿ�ָ��(������Ϊԭ�ӿ�ָ����Ҫ����Ϊ���°汾����ʹ��)
	mDirectSoundBuf8 = NULL;
	if(FAILED(mDirectSoundBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void **)&mDirectSoundBuf8)))
	{
		mDirectSoundBuffer->Release();
		return false;
	}

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ���ȡ��DirectSound�������ӿ�ָ��");
	
	// ���û�����֪ͨ�¼�
	for(int i = 0; i < CGlobalVariable::instance().gDsoundBufNotifyCount; i++)
	{
		mDsbPosEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		mDsbPosNotify[i].dwOffset = i * CGlobalVariable::instance().gDsoundBufNotifySize;
		mDsbPosNotify[i].hEventNotify = mDsbPosEvent[i];
	}

	if (FAILED(mDirectSoundBuf8->QueryInterface(IID_IDirectSoundNotify, (VOID**)&mDirectSoundNotify8)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ���ȡ��DirectSound�������¼�֪ͨ�ӿ�");

	if (FAILED(mDirectSoundNotify8->SetNotificationPositions(CGlobalVariable::instance().gDsoundBufNotifyCount, mDsbPosNotify)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ�����DirectSound�������¼�");

	mDirectSoundNotify8->Release();

	// ----
	// �Ȳ���һ�ο������Լ���һ��֪ͨ�¼�,���򲥷ź�����WaitForMultipleObjects����Զ����ʧ��.
	playnull();

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѳɹ�����һ�ο������Լ���һ��֪ͨ�¼�");

	mNextOffset = 0;

	mInitialized = true;

	return true;
}

/**
 *@name       free
 *@brief      ����ʼ����������
 *@param      �ޡ�
 *@return     �ޡ�
 */
void CAudioSounder::free()
{
	if (!mInitialized)
	{
		return;
	}

	if (NULL != mDirectSoundBuf8)
	{
		mDirectSoundBuf8->Stop();
	}

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "��ֹͣ���PCM������������Ƶ�豸��");

	for (int i = 0; i < CGlobalVariable::instance().gDsoundBufNotifyCount; i++)
	{
		mDsbPosNotify[i].dwOffset = 0;
		mDsbPosNotify[i].hEventNotify = NULL;
		CloseHandle(mDsbPosEvent[i]);
		mDsbPosEvent[i] = NULL;
	}

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "�ѹر��뻺�����󶨵��¼������");

	mNextOffset = 0;

	SAFE_RELEASE(mDirectSoundBuf8);
	SAFE_RELEASE(mDirectSoundBuffer);
	SAFE_RELEASE(mDirectSound);

	mInitialized = false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "���ͷŷ����DirectSound��������");
}

/**
 *@name       sound
 *@brief      ����PCM������������Ƶ����豸��
 *@param[in]  pcmdata pcm���ݵ�ַ��
 *@param[in]  pcmsize pcm���ݳ��ȡ�
 *@return     true��ʾ�ɹ���false��ʾʧ�ܡ�
 */
int CAudioSounder::sound(const void *pcmdata, const unsigned long &pcmsize)
{
	if (!mInitialized || pcmdata == 0 || pcmsize == 0)
	{
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "!mInitialized || pcmdata == 0 || pcmsize == 0, |pcmdata|pcmsize| -> |%ld|%lu|", pcmdata, pcmsize);
		return -1;
	}

	unsigned long  cpysize = 0;
	HRESULT hr = S_OK;
	BYTE *dwPcmPtr1 = NULL;
	BYTE *dwPcmPtr2 = NULL;
	DWORD dwPcmSize1 = 0;
	DWORD dwPcmSize2 = 0;
	DWORD dwNotifiedObjects = 0;
	LONG  iLockSize = 0;
	unsigned long cpysize2 = 0;

	dwNotifiedObjects = ::WaitForMultipleObjects(CGlobalVariable::instance().gDsoundBufNotifyCount, mDsbPosEvent, FALSE, CGlobalVariable::instance().gDsoundBufNotifyTimeout);
	//CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "����һ֡pcm�ĺ�ʱΪ:%003dms", playEnd - playStart);

	if((dwNotifiedObjects < WAIT_OBJECT_0) || (dwNotifiedObjects > WAIT_OBJECT_0 + (CGlobalVariable::instance().gDsoundBufNotifyCount-1)))
	{
		if (WAIT_TIMEOUT == dwNotifiedObjects)
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "�ȴ�DSound������֪ͨ�¼���ʱ���²���PCM����ʧ�ܣ�NotifiedObjects=%d,Count=%d,Timeout=%d", dwNotifiedObjects, CGlobalVariable::instance().gDsoundBufNotifyCount, CGlobalVariable::instance().gDsoundBufNotifyTimeout);
		}
		else if (WAIT_FAILED == dwNotifiedObjects)
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "�ȴ�DSound������֪ͨ�¼�����δ֪�����²���PCM����ʧ�ܣ�");
		}
		else
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "�ȴ�DSound������֪ͨ�¼�ʧ�ܣ�δ֪ԭ��");
		}
		
		playnull();

		return -2;
	}
	if (pcmsize == 0)
		iLockSize = CGlobalVariable::instance().gDsoundBufNotifySize;
	else
		iLockSize = pcmsize;

	hr = mDirectSoundBuf8->Lock(mNextOffset, iLockSize, (void **)&dwPcmPtr1, &dwPcmSize1, (void **)&dwPcmPtr2, &dwPcmSize2, 0);

	if(hr == DSERR_BUFFERLOST)
	{
		mDirectSoundBuf8->Restore();
		mDirectSoundBuf8->Lock(mNextOffset, iLockSize, (void **)&dwPcmPtr1, &dwPcmSize1, (void **)&dwPcmPtr2, &dwPcmSize2, 0);
	}
	if (hr == DSERR_INVALIDPARAM)
	{
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "DirectSoundBuf8->Lock failed. hr = %ld, An invalid parameter was passed to the returning function", hr);
	}
	if((dwPcmPtr1 == NULL) || (dwPcmSize1 == 0))
	{
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "(dwPcmPtr1 == NULL) || (dwPcmSize1 == 0)�� dwPcmPtr1=%d, dwPcmSize1=%d", dwPcmPtr1, dwPcmSize1);
		return -3;
	}
	if(SUCCEEDED(hr))
	{
		// �˴���Ҫ����:��Դ���������ݳ��ȴ����������ȡ�Ļ��������ȣ�
		// ����ȡСԭ�򿽱����ݣ�Դ�������Ქ�Ų�����������ڷ��仺��
		// ����ʱ��Ҫ�����㹻��

		// copy buffer1
		if (dwPcmPtr1 != NULL)
		{
			CopyMemory(dwPcmPtr1, pcmdata, dwPcmSize1);
			mNextOffset = mNextOffset + dwPcmSize1;
		}

		// copy buffer2
		if (dwPcmPtr2 != NULL)
		{
			if(pcmsize > dwPcmSize1)
			{
				cpysize2 = pcmsize - dwPcmSize1;
				CopyMemory(dwPcmPtr2, (BYTE*)pcmdata + dwPcmSize1, dwPcmSize2);
				mNextOffset = mNextOffset + dwPcmSize2;
			}
		}

		mNextOffset = mNextOffset % (CGlobalVariable::instance().gDsoundBufNotifySize * CGlobalVariable::instance().gDsoundBufNotifyCount);

		// д����ɺ����
		mDirectSoundBuf8->Unlock(dwPcmPtr1, dwPcmSize1, dwPcmPtr2, dwPcmSize2);

		mDirectSoundBuf8->Play(0, 0, DSBPLAY_LOOPING);//����looping,��;�ر���Ƶʱ�������һֱ����ĩβ������
		//mDirectSoundBuf8->Play(0, 0, 0);//��ʹ��looping,������������һ��һ��
	}

	return 0;
}


void CAudioSounder::mute(bool bMute)
{
	if (!mInitialized)
	{
		return;
	}

	if(mDirectSoundBuf8 != NULL)
	{
		if(bMute)
		{
			mDirectSoundBuf8->GetVolume(&mVolume);
			mDirectSoundBuf8->SetVolume(-10000);
		}
		else
		{
			mDirectSoundBuf8->SetVolume(mVolume);
		}
	}
}

/**
 *@name       stop
 *@brief      ֹͣ���š�
 *@param      �ޡ�
 *@return     �ޡ�
 */
void CAudioSounder::stop()
{
	if (!mInitialized)
	{
		return;
	}

	if (NULL != mDirectSoundBuf8)
	{
		mDirectSoundBuf8->Stop();
	}
}

void CAudioSounder::playnull()
{
	if (!mInitialized)
	{
		return;
	}
	BYTE *dwBufLock = NULL;
	DWORD dwBufSize = 0;
	if (FAILED(mDirectSoundBuf8->Lock(0, 0, (void **)&dwBufLock, &dwBufSize, NULL, NULL, DSBLOCK_ENTIREBUFFER)))
	{
		return;
	}
	memset(dwBufLock, 0, dwBufSize);
	if (FAILED(mDirectSoundBuf8->Unlock(dwBufLock, dwBufSize, NULL, 0)))
	{
		return;
	}
	if (FAILED(mDirectSoundBuf8->SetCurrentPosition(0)))
	{
		return;
	}
	if (FAILED(mDirectSoundBuf8->Play(0, 0, 0)))
	{
		return;
	}
}