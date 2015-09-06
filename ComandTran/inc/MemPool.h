#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_
/*------------------Include-------------------------*/
#include "DataStructDef.h"
#include "MemBlockListManager.h"
/*------------------Define-------------------------*/
#define GROW_COUNT_MAX 32
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
    int GetSize();
private:
    virtual void FreeMem();
    ContnBlockInf* GetContnBlock(int blockSize, int blockCount);
    bool ExtendPool(int size, int count);
public:
    int m_BlockCount;//current block size
	
private:
    long BlockSize;
    long BlockCount;
    int GrowStep;
    int GrowCount;
    bool CreatedFlag;
    ContnBlockInf *ContnBlockList;
    CMemBlockListManager FreeList;           
    CMemBlockListManager UsedList;
};
#endif
