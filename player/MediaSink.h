#ifndef MEDIA_SINK_H
#define MEDIA_SINK_H

#include "MediaInfo.h"


///ý�����ݴ���
class  TMediaSink
{
public:
	virtual ~TMediaSink() {}

	///����ý���ʽ
	virtual bool open() =0;

	///����ý������
	virtual bool writeFrame(const JPlayer_MediaFrame& frame) =0;

	///֪ͨ����ý�����ݽ���
	virtual void close() =0;

};



#endif



