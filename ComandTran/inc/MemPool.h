#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_
/*------------------Include-------------------------*/
#include "DataStructDef.h"
#include "MemBlockListManager.h"
#include "LockFreeQueue.h"

/*------------------Define-------------------------*/
#define GROW_COUNT_MAX 20
struct ContnBlockInf
{
	MemBlock *pMemBlockList;
	char *pMemBlock;
	ContnBlockInf *pNext;
};

class CMemPool
{
public:
    CMemPool();
    ~CMemPool();
    bool CreatPool(int blockSize, int blockCount, int step);
    char* GetBlock();
    bool FreeBlock(char *addr);
    long GetBlockSize();
    int GetBlockTotalCount();
    int GetBlockFreeCount();
private:
    virtual void FreeMem();
    bool ExtendPool(int size, int count);
public:
    int m_BlockTotalCount;//current block size
	
private:
    long BlockSize;
    long BlockCount;
    int GrowStep;
    int GrowCount;
    bool CreatedFlag;
    CLockFreeQueue<char*>* UnUsedBlockQueue;
};
#endif
