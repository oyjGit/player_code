#ifndef XALLOCATOR_H
#define XALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct  
{
	size_t size;
	size_t prev;
	size_t next;
} xmm_block_t;


typedef struct  
{
	size_t     abssize;
	size_t     firstused;
	size_t     firstfree;
} xmm_hdr_block_t;


class XMObject 
{  
public:
	xmm_hdr_block_t *base;
	size_t size;    
};

class MemXAllocator
{
public:
	MemXAllocator();
	~MemXAllocator();
	
     bool CreatePool(size_t maxLength);
     void DestroyPool();   

     void *Malloc(size_t reqsize);
     void *Calloc(size_t reqsize);
     void  Free(void *entity);

	 inline size_t GetPoolSize(){return m_xm.size;}

private:
	size_t FindBlockBySize(size_t size);
	size_t FindBlockByOffset(size_t next, size_t find, int includes);
	void   MoveBlock(size_t t, int f);
	
private:
	 unsigned char *m_baseMemory;	 
	 XMObject m_xm;	 	

private:
	/// disable assign and copy
	MemXAllocator(const MemXAllocator& ref);
	MemXAllocator& operator =(const MemXAllocator& ref);

};



#endif /* XALLOCATOR_H */
