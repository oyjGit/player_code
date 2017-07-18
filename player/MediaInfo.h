#ifndef MEDIA_INFO_H
#define MEDIA_INFO_H

#include <string.h>



////基础数据类型
typedef unsigned char            uint8_t;
typedef unsigned short           uint16_t;
typedef long long                int64_t;
typedef unsigned long long       uint64_t;
typedef unsigned int             uint32_t;


typedef enum 
{
	JPLAYER_SOFT_DECODER = 0,
	JPLAYER_OMX_DECODER,
	JPLAYER_XBMC_DECODER
}JPlayer_Decoder_TYPE;


///媒体数据发送结构
typedef struct 
{
	uint8_t* data;      ///媒体数据
	int      size;      ///媒体数据长度
	int      flag;      ///媒体帧类型
	int      seq;
	int      isKey;
	uint64_t timestamp; ///数据帧产生的时间，milliseconds    
}StreamPacket;




///媒体数据帧类型
typedef enum 
{
	JPLAYER_FRAME_PCM      = 1, 
	JPLAYER_FRAME_YUV420P  = 2,
	JPLAYER_FRAME_YUV12    = 4,
	JPLAYER_FRAME_AUDIO    = 8,
	JPLAYER_FRAME_H264     = 16,

	JPLAYER_FRAME_LENGTH
}JPlayer_FrameType;


typedef struct 
{
	uint8_t* m_pData[3];       // 数据指针
	int   m_size[3];           // 数据长度
	int   m_width;             // 图像宽
	int   m_height;            // 图像高
	int   m_flag;              // 数据类型
	uint64_t m_timestamp;      ///数据帧产生的时间，milliseconds 
	int   m_dts;
	uint64_t m_serialNum;
} JPlayer_MediaFrame;




#endif

