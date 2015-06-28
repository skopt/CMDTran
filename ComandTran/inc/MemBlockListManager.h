#ifndef _MEMBLCOKLISTMANAGER_H_
#define _MEMBLCOKLISTMANAGER_H_
/*------------------Include-------------------------*/
#include "DataStructDef.h"


/*------------------Define-------------------------*/


class CMemBlockListManager
{
public:
    CMemBlockListManager();
	~CMemBlockListManager();
	MemBlock* GetBlockHead();
	bool PushHead(MemBlock *pBlock);
	bool PushTrail(MemBlock *pBlock);
	MemBlock* DeletBlockWithAddr(char *addr);
	bool IsEmpty();
private:

public:
	int m_BlockCount;//current block size
	
private:
	MemBlock *pListHead;
	MemBlock *pListTrail;

};


#endif
