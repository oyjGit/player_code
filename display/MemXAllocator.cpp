
#include "MemXAllocator.h"

#include "Logs.h"


#define XAPR_ALIGN(size, boundary) (((size) + ((boundary) - 1)) & ~((boundary) - 1))
#define XAPR_ALIGN_DEFAULT(size)   XAPR_ALIGN(size, 4)

#define XMM_HDR_BLOCK_SIZE  (XAPR_ALIGN_DEFAULT(sizeof(xmm_hdr_block_t)))
#define XMM_BLOCK_SIZE      (XAPR_ALIGN_DEFAULT(sizeof(xmm_block_t)))



MemXAllocator::MemXAllocator()
	:m_baseMemory(NULL)
{

}

MemXAllocator::~MemXAllocator()
{
	DestroyPool();

}

size_t MemXAllocator::FindBlockByOffset(size_t next, size_t find, int includes)
{
    size_t prev = 0;

    while (next) 
	{
        xmm_block_t *blk = (xmm_block_t*)((unsigned char *)m_xm.base + next);

        if (find == next)
		{
            return next;
		}

        /* Overshot? */
        if (find < next)
		{
            return includes ? prev : 0;
		}

        prev = next;
        next = blk->next;
    }
    return includes ? prev : 0;
}

size_t MemXAllocator::FindBlockBySize(size_t size)
{
    size_t next = m_xm.base->firstfree;
    size_t best = 0;
    size_t bestsize = 0;

    while (next) 
	{
        xmm_block_t *blk = (xmm_block_t*)((unsigned char *)m_xm.base + next);

        if (blk->size == size)
		{
            return next;
		}

        if (blk->size >= size) 
		{
    
            if (!bestsize || (blk->size < bestsize)) 
			{
                bestsize = blk->size;
                best = next;
            }
        }

        next = blk->next;
    }

    if (bestsize > XMM_BLOCK_SIZE + size) 
	{
        xmm_block_t *blk = (xmm_block_t*)((unsigned char *)m_xm.base + best);
        xmm_block_t *neww = (xmm_block_t*)((unsigned char *)m_xm.base + best + size);

        neww->size = blk->size - size;
        neww->next = blk->next;
        neww->prev = best;

        blk->size = size;
        blk->next = best + size;

        if (neww->next) 
		{
            blk = (xmm_block_t*)((unsigned char *)m_xm.base + neww->next);
            blk->prev = best + size;
        }
    }

    return best;
}

void MemXAllocator::MoveBlock(size_t t, int f)
{   
    xmm_block_t *blk = (xmm_block_t*)((unsigned char *)m_xm.base + t);

    /* close the gap */
    if (blk->prev) 
	{
        xmm_block_t *prev = (xmm_block_t*)((unsigned char *)m_xm.base + blk->prev);
        prev->next = blk->next;
    }
    else
	{
        if (f) 
		{
            m_xm.base->firstused = blk->next;
        }
        else 
		{
            m_xm.base->firstfree = blk->next;
        }
    }

    if (blk->next) 
	{
        xmm_block_t *next = (xmm_block_t*)((unsigned char *)m_xm.base + blk->next);
        next->prev = blk->prev;
    }

    /* now find it in the other list, pushing it to the head if required */
    if (f) 
	{
        blk->prev = FindBlockByOffset(m_xm.base->firstfree, t, 1);
        if (!blk->prev) 
		{
            blk->next = m_xm.base->firstfree;
            m_xm.base->firstfree = t;
        }
    }
    else 
	{
        blk->prev = FindBlockByOffset(m_xm.base->firstused, t, 1);
        if (!blk->prev) 
		{
            blk->next = m_xm.base->firstused;
            m_xm.base->firstused = t;
        }
    }

    /* and open it up */
    if (blk->prev) 
	{
        xmm_block_t *prev = (xmm_block_t*)((unsigned char *)m_xm.base + blk->prev);
        if (f && (blk->prev + prev->size == t)) 
		{
            /* Collapse us into our predecessor */
            prev->size += blk->size;
            t = blk->prev;
            blk = prev;
        }
        else 
		{
            blk->next = prev->next;
            prev->next = t;
        }
    }

    if (blk->next) 
	{
        xmm_block_t *next = (xmm_block_t*)((unsigned char *)m_xm.base + blk->next);
        if (f && (t + blk->size == blk->next)) 
		{
            /* Collapse us into our successor */
            blk->size += next->size;
            blk->next = next->next;
            if (blk->next) 
			{
                next = (xmm_block_t*)((unsigned char *)m_xm.base + blk->next);
                next->prev = t;
            }
        }
        else 
		{
            next->prev = t;
        }
    }
}

bool MemXAllocator::CreatePool(size_t maxLength)
{
	if (m_baseMemory != NULL)
	{
		return false;
	}

	if (maxLength <= XMM_HDR_BLOCK_SIZE + XMM_BLOCK_SIZE)
	{
		return false;
	}


	m_baseMemory = new unsigned char[maxLength];
	if (m_baseMemory == NULL)
	{		
		return false;
	}	
   
    m_xm.base =(xmm_hdr_block_t *) m_baseMemory;
    m_xm.size = maxLength;  

    m_xm.base->abssize = maxLength;
    m_xm.base->firstused = 0;
    m_xm.base->firstfree = XMM_HDR_BLOCK_SIZE;

    xmm_block_t *blk = (xmm_block_t *)(m_baseMemory + m_xm.base->firstfree);
    blk->size = maxLength - m_xm.base->firstfree;
    blk->prev = 0;
    blk->next = 0;	

    return true;
}

void MemXAllocator::DestroyPool()
{    
	if ( m_baseMemory == NULL)
	{
		return;
	}

    xmm_block_t *blk;   

    if (m_xm.base->firstused) 
	{
        size_t t = m_xm.base->firstused;
        do {
            blk = (xmm_block_t *)((unsigned char*)m_xm.base + t);
            t = blk->next;
            blk->next = blk->prev = 0;
        } while (t);
        m_xm.base->firstused = 0;
    }

    if (m_xm.base->firstfree)
	{
        size_t t = m_xm.base->firstfree;
        do {
            blk = (xmm_block_t *)((unsigned char*)m_xm.base + t);
            t = blk->next;
            blk->next = blk->prev = 0;
        } while (t);
       m_xm.base->firstfree = 0;
    }
   
	m_xm.base->abssize = 0;
	m_xm.size = 0;  

	if (m_baseMemory != NULL)
	{		
		delete []m_baseMemory;
		m_baseMemory = NULL;		
	}	
}



void *MemXAllocator::Malloc(size_t reqsize)
{
	if (m_baseMemory == NULL)
	{
		JPLAYER_LOG_INFO("MemXAllocator  m_baseMemory NULL");
		return NULL;
	}

    size_t size;
    size_t t;
    
    size = XAPR_ALIGN_DEFAULT(reqsize) + XMM_BLOCK_SIZE;
    if (size < reqsize) 
	{
//		JPLAYER_LOG_INFO("MemXAllocator  size :%d   %d",size ,reqsize);
        return NULL;
    }  

    t = FindBlockBySize(size);
	if (t == 0)
	{
//		JPLAYER_LOG_INFO("MemXAllocator  t == 0");
		return NULL;
	}
    
    MoveBlock(t, 0);
    t += XMM_BLOCK_SIZE;  	
   
    return (void*)((unsigned char*)m_xm.base + t);
}

void *MemXAllocator::Calloc(size_t reqsize)
{
	size_t size = XAPR_ALIGN_DEFAULT(reqsize) + XMM_BLOCK_SIZE;
	char *ptr =(char *)Malloc(reqsize);
	if (ptr != NULL)
	{
		memset(ptr, 0, size - XMM_BLOCK_SIZE);
	}      
    return (void*)ptr;
}



void MemXAllocator::Free(void *entity)
{   
	if (m_baseMemory == NULL)
	{
		return ;
	}

	if (entity == NULL)
	{
		return;
	}

   
	size_t t = ((unsigned char*)entity -(unsigned char *) m_xm.base);  
    if (t < XMM_HDR_BLOCK_SIZE + XMM_BLOCK_SIZE) 
	{
        return ;
    }

    t -= XMM_BLOCK_SIZE;

     xmm_block_t *blk = (xmm_block_t*)((unsigned char*)m_xm.base + t);   
    if (blk->prev) 
	{
        xmm_block_t *prev = (xmm_block_t*)((unsigned char *)m_xm.base + blk->prev);
        if (prev->next != t) 
		{           
            return ;
        }
    }
    else 
	{
        if (m_xm.base->firstused != t) 
		{           
            return ;
        }
    }

    if (blk->next) 
	{
        xmm_block_t *next = (xmm_block_t*)((unsigned char *)m_xm.base + blk->next);
        if (next->prev != t) 
		{           
            return ;
        }
    }

    MoveBlock(t, 1);    
    return ;
}


