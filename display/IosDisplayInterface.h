#ifndef IOS_JPLAYER_SDK_H_
#define IOS_JPLAYER_SDK_H_

#include "MediaInfo.h"


#ifdef __cplusplus
extern "C"
{
#endif



enum
{
	DISPLAY_PLAYER_CLOSED  = -3,
	DISPLAY_PLAYER_EMPTY   = -2,
	DISPLAY_PLAYER_ERROR   = -1,
	DISPLAY_PLAYER_OK
};


int  getYUV420PFrame(int millisecond, JPlayer_MediaFrame *pPic);

void releaseYUV420PFrame(JPlayer_MediaFrame *pPic);


#ifdef __cplusplus
}
#endif

#endif //!IOS_JPLAYER_SDK_H_




