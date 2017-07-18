
#ifndef TYUV2_QUEUE_H_
#define TYUV2_QUEUE_H_

#include "TCriticalSection.h"
#include "TConditionVariable.h"
#include "MemXAllocator.h"
#include "MediaInfo.h"
#include <deque>
#include "Logs.h"

namespace comn
{

class CYuvMemQueue
{
public:		
	CYuvMemQueue()
		:m_maxMemLength(6 * 1024 * 1024),
		 m_signaled(false)
	{
		m_queue.clear();
	}

	~CYuvMemQueue()
	{
		unInit();
	}


	bool init(size_t maxMemLength)
	{
		comn::AutoCritSec tmpLock(m_csPool);
		m_maxMemLength = maxMemLength;
		m_signaled = false;

		return m_memPool.CreatePool(m_maxMemLength);
	}

	void unInit()
	{
		comn::AutoCritSec tmpLock(m_csPool);
		releaseAll();
		m_signaled = false;

		m_memPool.DestroyPool();
	}


	bool push(const JPlayer_MediaFrame &packet,bool hasrelease = false)
	{
		bool ok = true;

		JPlayer_MediaFrame frame = packet;

		int picSize = 1920 * 1080;
		if (packet.m_width>1920||packet.m_height>1080)
			picSize = packet.m_width * packet.m_height;
		int newSize = picSize*3/2;

		unsigned char *pYuv = NULL;

		{
			comn::AutoCritSec tmpLock(m_csPool);
			//�����ڴ�
			pYuv = (unsigned char *)m_memPool.Malloc(newSize);
			if (pYuv == NULL)
			{
				if (hasrelease)
				{
					releaseFront();
					JPLAYER_LOG_ERROR("yuv queue releaseFront\n");

					pYuv = (unsigned char *)m_memPool.Malloc(newSize);
					if (pYuv == NULL)
					{
						releaseAll();
						JPLAYER_LOG_ERROR("yuv queue releaseAll\n");
						pYuv = (unsigned char *)m_memPool.Malloc(newSize);
					}
				}

				if (pYuv == NULL)
				{
					ok = false; 
					return ok;
				}
			}
		}


		int j = 0,i = 0; 
		for (i = 0; i < frame.m_height; i++) 
		{ 
			memcpy(pYuv + j,frame.m_pData[0] + i*frame.m_size[0], frame.m_width); 
			j += frame.m_width; 
		} 

		for (i = 0; i < frame.m_height/2; i++) 
		{ 
			memcpy(pYuv + j,frame.m_pData[1] + i*frame.m_size[1], frame.m_width/2); 
			j += frame.m_width/2;
		} 

		for (i = 0; i < frame.m_height/2; i++) 
		{ 
			memcpy(pYuv + j,frame.m_pData[2] + i*frame.m_size[2], frame.m_width/2); 
			j += frame.m_width/2;
		}

		frame.m_pData[0] = pYuv;
		frame.m_size[0]  = j;
		frame.m_pData[1] = NULL;
		frame.m_size[1] = 0;
		frame.m_pData[2] = NULL;
		frame.m_size[2] = 0;


		{
			comn::AutoCritSec tmpLock(m_csQueue);
			m_queue.push_back(frame);	///��ݷ������
		}

			
		m_cond.signal(); 

		return ok;
	}

	bool tryPop(JPlayer_MediaFrame &frame)
	{
		comn::AutoCritSec  tmpLock(m_csQueue);
		if (m_queue.empty()) 
		{	
			return false;
		}	

		frame = m_queue.front();  ///ȡͷԪ��
		m_queue.pop_front();      ///����ͷԪ��

		return true;
	}

	bool pop(JPlayer_MediaFrame &frame)
	{
		comn::AutoCritSec  tmpLock(m_csQueue);
		while (m_queue.empty()) 
		{	
			//�߳�˯�߽���ȴ��������ݱ�����
			m_cond.wait(m_csQueue);  
		}	
		frame = m_queue.front();  ///ȡͷԪ��
		m_queue.pop_front();      ///����ͷԪ��

		return true;
	}


	bool popTimeWait(JPlayer_MediaFrame &item,int millisec)
	{
		comn::AutoCritSec  tmpLock(m_csQueue);

		while (m_queue.empty()) 
		{	
			if (m_signaled)
			{
				m_signaled = false;
				return false;
			}

			//�߳�˯�߽���ȴ��������ݱ����ѻ��߳�ʱ
			int ret = m_cond.timedwait(m_csQueue,millisec);
			if (ret != CVariable_Success)      ///��ʱ��
			{	
				//JPLAYER_LOG_ERROR("popTimeWait ret:%d millisec:%d\n",ret,millisec);
				return false;
			}	
			else
			{
				if(m_queue.empty())
				{
					return false;
				}
			}
		}

		item = m_queue.front();  ///ȡͷԪ��
		m_queue.pop_front();

		return true;
	}

	
	void release(JPlayer_MediaFrame &frame)
	{
		comn::AutoCritSec tmpLock(m_csPool);
		m_memPool.Free(frame.m_pData[0]);
		frame.m_pData[0] = NULL;
	}


	

	size_t  size()
	{
		comn::AutoCritSec tmpLock(m_csQueue);
		return m_queue.size();	
	}

	bool  empty()
	{
		comn::AutoCritSec  tmpLock(m_csQueue);
		return m_queue.empty();
	}

	bool  signalQueue()
	{
		m_cond.signal();

		comn::AutoCritSec  tmpLock(m_csQueue);
		m_signaled = true;
		return true;
	}


	void releaseAllBuffer()
	{	
		comn::AutoCritSec  tmpLock1(m_csQueue);
		comn::AutoCritSec  tmpLock2(m_csPool);
		while(!m_queue.empty())
		{
			JPlayer_MediaFrame item = m_queue.front();  ///ȡͷԪ��
			if (item.m_pData != NULL)
			{
				m_memPool.Free(item.m_pData[0]);
			}

			m_queue.pop_front();
		}
	}

private:
	void releaseFront()
	{
		comn::AutoCritSec  tmpLock(m_csQueue);

		if(!m_queue.empty())
		{
			JPlayer_MediaFrame frame = m_queue.front();  ///ȡͷԪ��
			m_memPool.Free(frame.m_pData[0]);
			m_queue.pop_front();
		}
	}


	void releaseAll()
	{	
		comn::AutoCritSec  tmpLockQueue(m_csQueue);
		while(!m_queue.empty())
		{
			JPlayer_MediaFrame item = m_queue.front();  ///ȡͷԪ��
			if (item.m_pData != NULL)
			{
				m_memPool.Free(item.m_pData[0]);
			}

			m_queue.pop_front();
		}
	}

private:		
	std::deque<JPlayer_MediaFrame>  m_queue;  ///stl deque
	comn::CriticalSection    m_csQueue ;           ///�߳���
	comn::ConditionVariable  m_cond;          ///��������
	comn::CriticalSection    m_csPool ;           ///�߳���
	MemXAllocator            m_memPool;       ///�ڴ����
	size_t                   m_maxMemLength;  ///�����ڴ��С
	bool m_signaled;

};


}

////////////////////////////////////////////////////////////////////////////
#endif /* TYUV_QUEUE_H_ */







