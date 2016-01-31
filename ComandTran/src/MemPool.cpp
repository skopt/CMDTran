#include "MemPool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "Log.h"

CMemPool::CMemPool()
:m_BlockTotalCount(0),CreatedFlag(false),GrowCount(0)
{
    UnUsedBlockQueue = new CLockFreeQueue<char*>();
}
CMemPool::~CMemPool()
{
    FreeMem();
    delete UnUsedBlockQueue;
}
//creat the pool according to the param
bool CMemPool::CreatPool(int blockSize, int blockCount, int step)
{
    BlockSize = blockSize;
    BlockCount = blockCount;
    GrowStep = step;
    if(BlockSize <= 0 || BlockCount <= 0 || CreatedFlag)
    {
    	return false;
    }

    //get a continue memery block	
    bool ret = ExtendPool(BlockSize, BlockCount);
    if(false == ret)
    {
    	LogError("CreatPool: extend pool failed, count=%d, free size=%d", 
    	     m_BlockTotalCount, UnUsedBlockQueue->Size());
    	return false;
    }
       
    m_BlockTotalCount += BlockCount;
    LogInf("ExtendPool succeed, the count is %d", m_BlockTotalCount);
    CreatedFlag = true;
    return true;
}
char* CMemPool::GetBlock()
{
    if(UnUsedBlockQueue->Size() <= 0)
    {
        //printf("need to exten\n");
        if(!ExtendPool(BlockSize,GrowStep))
        {
            LogInf("Extend Pool faild, curren block cout=%d, free size=%d", m_BlockTotalCount, UnUsedBlockQueue->Size());
            return NULL;
        }
         m_BlockTotalCount += GrowStep;
         LogInf("Extend Pool succeed, curren block cout is  %d", m_BlockTotalCount);
    }
     
    char* ret = NULL;
    if(!UnUsedBlockQueue->Pop(ret))
        return NULL;

    return ret;
}

bool CMemPool::FreeBlock(char *addr)
{
    return UnUsedBlockQueue->Push(addr);
}

long CMemPool::GetBlockSize()
{
    return BlockSize;
}
int CMemPool::GetBlockTotalCount()
{
    return m_BlockTotalCount;
}

int CMemPool::GetBlockFreeCount()
{
    return UnUsedBlockQueue->Size();
}


/*****************not public*************************/
void CMemPool::FreeMem()
{
    char* buff = NULL;
    while(UnUsedBlockQueue->Size() > 0)
    {
         if(UnUsedBlockQueue->Pop(buff))
            delete[] buff;
    }
}

bool CMemPool::ExtendPool(int size, int count)
{
    if(GrowCount >= GROW_COUNT_MAX)
    {
        LogError("Grow Count max");
        return false;
    }

    GrowCount++;

    for(int i = 0; i < count; ++i)
    {
        char* buff = new char[size];
        if(false == UnUsedBlockQueue->Push(buff))
            LogError("push failed\n");
    }

    LogInf("Extend pool, unused size=%d\n", UnUsedBlockQueue->Size());
    return true;
}


