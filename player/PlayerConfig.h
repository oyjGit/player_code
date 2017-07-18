#ifndef CONFIG_INFO_H
#define CONFIG_INFO_H

#define  KB   (1024)
#define  MB   (KB * KB)
#define  GB   (MB * KB)


#if defined(XBMC_DECODE) && defined(OMX_DECODE)
#error unsupported decoder
#endif

#if defined(XBMC_DECODE) || defined(OMX_DECODE)
#define USE_HARD_DECODER
#endif


enum JPlayerConfig
{
	 VIDEO_MAX_PACKET_POOL     = 4 ,
	 VIDEO_DISPLAY_WAITTIME    = 200,
	 YUV_MAX_FRAME_POOL        = 6,

	 AUDIO_MAX_PACKET_POOL     = 50,
	 AUDIO_BIT_RATES           = 16000,
	 AUDIO_DELAY_TIME          = 1000,
	 AUDIO_SAMPLE_RATES        = 48000,
	 AUDIO_CHANNEL             = 1,

	 VOD_MAX_PACKET_POOL       = 30,
	 VOD_MAX_INTERVAL_TIME     = 1000,
	 VOD_DEFAULT_INTERVAL_TIME = 70,
	 VOD_FREQUENCY_TIME        = 30000,
	 VOD_MIN_FRAME             = 320,

	 JPLAYER_CONFIG
};


#endif

