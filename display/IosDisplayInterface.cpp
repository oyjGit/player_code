#include "IosDisplayInterface.h"
#include "GlobalFunc.h"



int getYUV420PFrame(int millisecond, JPlayer_MediaFrame *pPic)
{
	if (pPic == NULL)
	{
		return DISPLAY_PLAYER_ERROR;
	}
	
	return getVideoManager().getYUV420PFrame(millisecond,*pPic);
}

void releaseYUV420PFrame(JPlayer_MediaFrame *pPic)
{
	if (pPic == NULL)
	{
		return;
	}

	getVideoManager().releaseYUV420PFrame(*pPic);

}

