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
 *@brief      初始化声音播放器。
 *@param[in]  WndHandle 用于接收事件的窗口句柄。
 *@param[in]  channels pcm的通道数。
 *@param[in]  SampleBits pcm的采样位数。
 *@param[in]  SampeRate pcm的采样率。
 *@return     true表示成功，false表示失败。
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

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功创建DirectSound对象");

	//设置协作级别.DSSCL_PRIORITY:程序独占;DSSCL_NORMAL:和其他程序共享
	if(FAILED(mDirectSound->SetCooperativeLevel(WndHandle, DSSCL_PRIORITY)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功设置DirectSound协作等级");

	DSCAPS dscaps;

	dscaps.dwSize = sizeof(DSCAPS);
	if (FAILED(mDirectSound->GetCaps(&dscaps)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功获取设备能力");

	mBufferDesc.dwSize = sizeof(DSBUFFERDESC);
	mBufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2| DSBCAPS_CTRLVOLUME | DSBCAPS_LOCSOFTWARE;
	mBufferDesc.dwBufferBytes = CGlobalVariable::instance().gDsoundBufNotifyCount * CGlobalVariable::instance().gDsoundBufNotifySize;
	mBufferDesc.lpwfxFormat = &mWaveFormat;

	// mDirectSoundBuffer为次缓冲区的接口指针,mark：主缓冲区？
	mDirectSoundBuffer = NULL;
	if(FAILED(mDirectSound->CreateSoundBuffer(&mBufferDesc, &mDirectSoundBuffer, NULL)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功创建DirectSound缓冲区. NotifySize = %d, NotifyCount = %d", CGlobalVariable::instance().gDsoundBufNotifySize, CGlobalVariable::instance().gDsoundBufNotifyCount);

	// mDirectSoundBuf8为最新版本次缓冲区的接口指针(可以认为原接口指针需要更新为最新版本才能使用)
	mDirectSoundBuf8 = NULL;
	if(FAILED(mDirectSoundBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void **)&mDirectSoundBuf8)))
	{
		mDirectSoundBuffer->Release();
		return false;
	}

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功获取到DirectSound缓冲区接口指针");
	
	// 设置缓冲区通知事件
	for(int i = 0; i < CGlobalVariable::instance().gDsoundBufNotifyCount; i++)
	{
		mDsbPosEvent[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		mDsbPosNotify[i].dwOffset = i * CGlobalVariable::instance().gDsoundBufNotifySize;
		mDsbPosNotify[i].hEventNotify = mDsbPosEvent[i];
	}

	if (FAILED(mDirectSoundBuf8->QueryInterface(IID_IDirectSoundNotify, (VOID**)&mDirectSoundNotify8)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功获取到DirectSound缓冲区事件通知接口");

	if (FAILED(mDirectSoundNotify8->SetNotificationPositions(CGlobalVariable::instance().gDsoundBufNotifyCount, mDsbPosNotify)))
		return false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功设置DirectSound缓冲区事件");

	mDirectSoundNotify8->Release();

	// ----
	// 先播放一次空数据以激活一个通知事件,否则播放函数中WaitForMultipleObjects将永远返回失败.
	playnull();

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功播放一次空数据以激活一个通知事件");

	mNextOffset = 0;

	mInitialized = true;

	return true;
}

/**
 *@name       free
 *@brief      反初始化声音器。
 *@param      无。
 *@return     无。
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

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已停止输出PCM声音数据至音频设备。");

	for (int i = 0; i < CGlobalVariable::instance().gDsoundBufNotifyCount; i++)
	{
		mDsbPosNotify[i].dwOffset = 0;
		mDsbPosNotify[i].hEventNotify = NULL;
		CloseHandle(mDsbPosEvent[i]);
		mDsbPosEvent[i] = NULL;
	}

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已关闭与缓冲区绑定的事件句柄。");

	mNextOffset = 0;

	SAFE_RELEASE(mDirectSoundBuf8);
	SAFE_RELEASE(mDirectSoundBuffer);
	SAFE_RELEASE(mDirectSound);

	mInitialized = false;

	CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已释放分配的DirectSound各个对象。");
}

/**
 *@name       sound
 *@brief      播放PCM声音数据于音频输出设备。
 *@param[in]  pcmdata pcm数据地址。
 *@param[in]  pcmsize pcm数据长度。
 *@return     true表示成功，false表示失败。
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
	//CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "播放一帧pcm的耗时为:%003dms", playEnd - playStart);

	if((dwNotifiedObjects < WAIT_OBJECT_0) || (dwNotifiedObjects > WAIT_OBJECT_0 + (CGlobalVariable::instance().gDsoundBufNotifyCount-1)))
	{
		if (WAIT_TIMEOUT == dwNotifiedObjects)
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "等待DSound缓冲区通知事件超时导致播放PCM声音失败！NotifiedObjects=%d,Count=%d,Timeout=%d", dwNotifiedObjects, CGlobalVariable::instance().gDsoundBufNotifyCount, CGlobalVariable::instance().gDsoundBufNotifyTimeout);
		}
		else if (WAIT_FAILED == dwNotifiedObjects)
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "等待DSound缓冲区通知事件发生未知错误导致播放PCM声音失败！");
		}
		else
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "等待DSound缓冲区通知事件失败！未知原因");
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
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "(dwPcmPtr1 == NULL) || (dwPcmSize1 == 0)， dwPcmPtr1=%d, dwPcmSize1=%d", dwPcmPtr1, dwPcmSize1);
		return -3;
	}
	if(SUCCEEDED(hr))
	{
		// 此处需要完善:若源声音的数据长度大于锁定后获取的缓冲区长度，
		// 根据取小原则拷贝数据，源声音将会播放不完整，因此在分配缓冲
		// 长度时需要分配足够。

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

		// 写入完成后解锁
		mDirectSoundBuf8->Unlock(dwPcmPtr1, dwPcmSize1, dwPcmPtr2, dwPcmSize2);

		mDirectSoundBuf8->Play(0, 0, DSBPLAY_LOOPING);//采用looping,中途关闭音频时，会出现一直播放末尾的声音
		//mDirectSoundBuf8->Play(0, 0, 0);//不使用looping,播放声音出现一断一断
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
 *@brief      停止播放。
 *@param      无。
 *@return     无。
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