#include "GlobalFunc.h"
#include "TSingleton.h"

class GlobalManager
{
public:
	AvSyncManager m_avSyncManager;
	VideoManager  m_videoManager;
	AudioManager  m_audioManager;
};

typedef comn::Singleton<GlobalManager> SingletonGlobalManager;

GlobalManager& getGlobalManager()
{
	return SingletonGlobalManager::instance();
}

AvSyncManager &getAvSyncManager()
{
	return getGlobalManager().m_avSyncManager;
}

AudioManager  &getAudioManager()
{
	return getGlobalManager().m_audioManager;
}

VideoManager  &getVideoManager()
{
	return getGlobalManager().m_videoManager;
}


