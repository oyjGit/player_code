#ifndef MEDIA_SINK_H
#define MEDIA_SINK_H

#include "MediaInfo.h"


///媒体数据处理
class  TMediaSink
{
public:
	virtual ~TMediaSink() {}

	///设置媒体格式
	virtual bool open() =0;

	///发送媒体数据
	virtual bool writeFrame(const JPlayer_MediaFrame& frame) =0;

	///通知发送媒体数据结束
	virtual void close() =0;

};



#endif



