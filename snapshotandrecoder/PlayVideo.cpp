#include "StdAfx.h"
#include "PlayVideo.h"
#include "LYCode.h"
#include "LogOutput.h"
#include "jpeglib.h"
#include "VideoDecoderFactory.h"

#pragma comment(lib, "libjpeg.lib")


CPlayVideo::CPlayVideo(void)
	: thread(false), CCallBackMsg(NULL)
, mFrameQue(NULL)
, mVideoWidth(0)
, mVideoHeight(0)
, mVideoDecodetype(0)
, mWndHandle(0)
, mEndThread(true)
, mAllFreed(true)
, mSaveJpeg(false)
, mSavePs(false)
, mSavePsOnce(false)
, mFileStream(NULL)
, mDrawFmt(PIX_FMT_NONE)
, mSystimeBase(0)
, mLasttimeStamp(0)
, mDeviceType(0)
, mPlayerCtrl(NULL)
, mPicture(nullptr)
, mImgConvertCtx(nullptr)
{
	memset(mFileName, 0, sizeof(mFileName));
}


CPlayVideo::~CPlayVideo(void)
{

}

bool CPlayVideo::startup(AVCodecID codecId, int VideoDecodetype, int WndHandle, CQueBuffer *FrameQue, CPlayerCallBack* cbObj,CPlayerControl* ctrl)
{
	if (codecId == AV_CODEC_ID_NONE || 0 >= WndHandle || NULL == FrameQue)
	{
		return false;
	}

	mVideoDecodetype = VideoDecodetype;
	mWndHandle = WndHandle;
	mEndThread = false;
	mFrameQue = FrameQue;
	mCbObj = cbObj;
	mPlayerCtrl = ctrl;

	if (!InitAll(codecId))
	{
		mEndThread = true;
		return false;
	}

	start();
	//running = 1;

	return true;
}

void CPlayVideo::endover()
{
	if (!mEndThread)
	{
		mEndThread = true;

		freed = 0;
		waitfor();
		running = 0;

		//FreeAll();
	}
}

void CPlayVideo::execute()
{
	char *data = NULL;
	long  size = 0;
	SMediaPacket packetImp;
	packetImp.bufSize = mFrameQue->getBufSize();
	packetImp.data = new char[packetImp.bufSize];
	SMediaPacket *packet = &packetImp;
	bool NeedFindkey(false);
	unsigned long exetime = timeGetTime();
	unsigned int firstFrameTime = 0;
	unsigned int lastVtime = 0;

	unsigned int timestamp = 0;
	unsigned int basetime = 0;
	unsigned int lastAtime = 0;
	int interval = 0;

	SFrameYUV frame;
	SFrameYUV *FrameYUV = &frame;
	SFrameYUV  dstFrame = { { nullptr, nullptr, nullptr, nullptr }, { 0, 0, 0, 0 } };
	bool       KeyFrame(false);
	int ctrlRet = 0;
	AVPixelFormat picPixFmt;
	unsigned long long LasttimeFailedV = timeGetTime();
	while (!mEndThread)
	{
#if 0
		if (timeGetTime() - exetime < 1000)
		{
			Sleep(5);
			continue;
		}
#endif
		int retPause = mPlayerCtrl->processPause(0);
		if (retPause == 1)
		{
			bool result = mFrameQue->pop(packetImp);
			if (!result)
			{
				Sleep(10);
				continue;
			}
			data = packet->data;
			size = packet->size;

			//如果没有暂停且没有等待I帧就解码
			bool gotKey = false;
			if (!mPlayerCtrl->mPause && (-1 != mPlayerCtrl->waitKeyFrame(*packet,gotKey)))
			{
				int ret = mVideoDecoder->decode((unsigned char *)data, size, FrameYUV, KeyFrame, picPixFmt);
				if (0 != ret)
				{
					//if (timeGetTime() - LasttimeFailedV > 5000)
					{
						char buf[256] = {0};
						av_strerror(ret, buf, 256);
						CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "解码视频数据失败,帧类型为:%d,size=%d,reason:%s,gotKey=%d", packet->frameType, packet->size, buf, gotKey);
						LasttimeFailedV = timeGetTime();
					}
					mPlayerCtrl->mWaitKeyFrame = true;
					Sleep(5);
					continue;
				}
			}
			ctrlRet = mPlayerCtrl->playVideo(*packet, mEndThread);
			if (ctrlRet == 0)
			{
				continue;
			}

			if (ctrlRet != 1000)
			{
				mVideoDecoder->getVideoSize(mVideoWidth, mVideoHeight);
				if (mDrawFmt != picPixFmt)
				{
					memset(&dstFrame, 0, sizeof(SFrameYUV));
					//if (yuv2rgb(FrameYUV->data, FrameYUV->linesize, FrameRGB.data, FrameRGB.linesize, mDrawRgbFmt))
					if (colorScale(FrameYUV, picPixFmt, dstFrame, mDrawFmt) != 0)
					{
						CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "转换视频格式失败.");
						Sleep(5);
						continue;
					}
				}
				else
				{
					memcpy(&dstFrame, FrameYUV, sizeof(SFrameYUV));
				}

				if (!mVideoDrawer.draw(&dstFrame, mVideoWidth, mVideoHeight))
				{
					Sleep(5);
					continue;
				}
			}
			mPlayerCtrl->updateVideoAfterPlay();
		}
		else
		{
			Sleep(50);
		}
		//TODO:计算截图需要的时间
		if (mSaveJpeg)
		{
			mVideoDecoder->getVideoSize(mVideoWidth, mVideoHeight);
			int err = CaptureJpg(FrameYUV, picPixFmt);
			if (0 != err)
			{
				char str[LY_MSG_MAX_LEN] = { 0 };
				sprintf_s(str, "save picture failed,errno:%d", err);
				CLogOutputEx::instance().output(OT_LOGFILE, LOG_INFO_ERROR, str);
				setCallBackMsg(EEID_SAVE_PIC_FAILED, str, strlen(str));
			}
			else
			{
				setCallBackMsg(EEID_SAVE_PIC_SUCCESS, &err, sizeof(err));
			}
			mSaveJpeg = false;
		}
	}
	FreeAll();
	if (packetImp.data)
	{
		delete packetImp.data;
	}
}

bool CPlayVideo::InitAll(AVCodecID codecId)
{
	do 
	{
		mVideoDecoder = CVideoDecoderFactory::createVideoDecoder(codecId, (VideoDecoderType)mVideoDecodetype);
		if (mVideoDecoder == nullptr)
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "创建解码器类型:%d失败\n", mVideoDecodetype);
			break;
		}
		if (LY_SUCCESS != mVideoDecoder->open(mDrawFmt))
		{
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "打开编码器(类型:%d)失败,色彩空间格式:%d\n", mVideoDecodetype, mDrawFmt);
			break;
		}

		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功初始化视频解码器");

		if (!mVideoDrawer.init((HWND)mWndHandle, mDrawFmt))
		{
			break;
		}
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已成功初始化图像绘制器");
		mAllFreed = false;
		return true;
	} while (0);
	FreeAll();
	return false;
}

void CPlayVideo::FreeAll()
{
	if (!mAllFreed)
	{
		mVideoDrawer.free();
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已释放图像绘制器。");

		if (mVideoDecoder != nullptr)
		{
			mVideoDecoder->close();
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "已释放视频解码器。");
		}

		if (nullptr != mPicture)
		{
			avpicture_free(mPicture);
			delete mPicture;
			mPicture = nullptr;
		}

		if (mImgConvertCtx != nullptr)
		{
			sws_freeContext(mImgConvertCtx);
			mImgConvertCtx = nullptr;
		}

		//ClosePs();
		mWndHandle  = 0;
		mSaveJpeg   = false;
		mSavePs     = false;
		mSavePsOnce = false;
		mHasAudio   = false;
		mDrawFmt = PIX_FMT_NONE;

		mAllFreed = true;
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_HIT, "视频播放部分关闭成功!。");
	}
}

// 保存jpg图片
bool CPlayVideo::savejpg(const char *FileName)
{
   if(!mEndThread)
   {
       if(0 == FileName)
	   {
		   return false;
	   }
	   strcpy(mFileJpeg, FileName);
	   mSaveJpeg = true;
	   return true;
   }

   return false;
}

// 抓取JPG图片
int CPlayVideo::CaptureJpg(SFrameYUV* src, PixelFormat srcFmt)
{
	FILE* file = fopen(mFileJpeg, "wb");
	int err = GetLastError();
	if (!file)
	{
		return err;
	}

	if (src->data[0] == nullptr || srcFmt == AV_PIX_FMT_NONE)
	{
		return -1;
	}

	AVPicture* pic = new AVPicture;
	avpicture_alloc(pic, AV_PIX_FMT_RGB24, mVideoWidth, mVideoHeight);

	struct SwsContext* sc = sws_getContext(mVideoWidth, mVideoHeight, srcFmt, mVideoWidth, mVideoHeight, AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	if (nullptr == sc)
	{
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "截图色彩空间转换器设置失败");
		return -2;
	}
	sws_scale(sc, src->data, src->linesize, 0, mVideoHeight, pic->data, pic->linesize);

	uint8_t* frameBuf = pic->data[0];
	int lineSize = pic->linesize[0];

	jpeg_compress_struct jpeg;
	jpeg_error_mgr jerr;
	jpeg.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&jpeg);

	jpeg_stdio_dest(&jpeg, file);
	jpeg.image_width = mVideoWidth;
	jpeg.image_height = mVideoHeight;
	jpeg.input_components = lineSize / mVideoWidth;
	jpeg.in_color_space = JCS_RGB;

	jpeg_set_defaults(&jpeg);

	// 开始压缩
	jpeg_start_compress(&jpeg, TRUE);

	JSAMPROW RowPointer[1];
	// 从上到下,设置图片中每一行的像素值
	while(jpeg.next_scanline < jpeg.image_height)
	{
		// 给一行数据赋值
		RowPointer[0] = &frameBuf[jpeg.next_scanline * lineSize];
		// 将一行数据写入
		jpeg_write_scanlines(&jpeg, RowPointer, 1);
	}

	// 结束压缩
	jpeg_finish_compress(&jpeg);
	// 清空对象
	jpeg_destroy_compress(&jpeg);

	fclose(file);
	file = NULL;

	sws_freeContext(sc);
	avpicture_free(pic);
	delete pic;

	return 0;
}

// 开始录像
bool CPlayVideo::saveps(const char *FileName)
{
	if (!mEndThread)
	{
		if (0 == FileName)
		{
			return false;
		}

		strcpy(mFilePs, FileName);

		mSavePsOnce = false;
		mSavePs = true;

		return true;
	}

	return false;
}

// 结束录像
void CPlayVideo::saveend()
{
	if (!mEndThread)
	{
		if (mSavePs)
		{
			ClosePs();
			mSavePsOnce = false;
			mSavePs = false;
		}
	}
}


void CPlayVideo::OpenPsForOnce()
{
	if (!mSavePsOnce)
	{
		mFileStream = fopen(mFilePs, "wb");
		if (0 != mFileStream)
		{
			mSavePsOnce = true;
		}
		else
		{
			mSavePsOnce = false;
			mSavePs = false;
		}
	}
}

bool CPlayVideo::WritePs(const char *FrameBuff, long FrameSize)
{
	int result = -1;
	if (0 != mFileStream)
	{
		result = fwrite(FrameBuff, sizeof(char), FrameSize, mFileStream);
	}

	return (0 > result);
}

void CPlayVideo::ClosePs()
{
	if (0 != mFileStream)
	{
		fclose(mFileStream);
		mFileStream = NULL;
	}
}

int CPlayVideo::colorScale(SFrameYUV* src,PixelFormat srcFmt ,SFrameYUV& dst,PixelFormat dstFmt)
{
	if (src == nullptr || nullptr == src->data[0])
	{
		CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "源数据为空");
		return -1;
	}

	if (nullptr == mPicture)
	{
		mPicture = new AVPicture;
		avpicture_alloc(mPicture, dstFmt, mVideoWidth, mVideoHeight);

		// setup scaler
		mImgConvertCtx = sws_getContext(mVideoWidth, mVideoHeight, srcFmt, mVideoWidth, mVideoHeight, dstFmt, SWS_FAST_BILINEAR, NULL, NULL, NULL);
		if (nullptr == mImgConvertCtx)
		{
			avpicture_free(mPicture);
			delete mPicture;
			mPicture = nullptr;
			CLogOutputEx::instance().output(OT_FILE_VIEWER, LOG_INFO_ERROR, "色彩空间转换器设置失败");
			return -1;
		}
	}
	
	if (nullptr != mImgConvertCtx)
	{
		// convert (src) to (dst)
		sws_scale(mImgConvertCtx, src->data, src->linesize, 0, mVideoHeight, mPicture->data, mPicture->linesize);

		dst.data[0] = mPicture->data[0];
		dst.linesize[0] = mPicture->linesize[0];
		dst.data[1] = mPicture->data[1];
		dst.linesize[1] = mPicture->linesize[1];
		dst.data[2] = mPicture->data[2];
		dst.linesize[2] = mPicture->linesize[2];
		return 0;
	}
	return -1;
}
