#include "MemPool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define LogE printf
#define LogI printf

CMemPool::CMemPool()
:m_BlockCount(0),CreatedFlag(false),GrowCount(0)
{
	ContnBlockList = NULL;
}
CMemPool::~CMemPool()
{
	FreeMem();
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
		LogE("CreatPool: extend pool failed");
		return false;
	}
       
       m_BlockCount += BlockCount;
       printf("ExtendPool succeed, the count is %d\n", m_BlockCount);
	CreatedFlag = true;
	return true;
}
char* CMemPool::GetBlock()
{
	MemBlock *pRet;
	pRet = FreeList.GetBlockHead();
	if(pRet == NULL && FreeList.IsEmpty())
	{
		if(!ExtendPool(BlockSize,GrowStep))
		{
			LogE("GetBlock: extend poll failed\n");
			return NULL;
		}             
		m_BlockCount += GrowStep;
             LogI("Extend Pool succeed, curren block cout is  %d\n", m_BlockCount);
		pRet = FreeList.GetBlockHead();
		if(NULL == pRet)
		{
			LogE("Get block: get null even extend the pool\n");
			return NULL;
		}
	}

	UsedList.PushTrail(pRet);
	if(NULL == pRet->pBlock)
	{
	    LogI("pRet->pBlock is null\n");
	    return NULL;
	}
	memset(pRet->pBlock, 0, BlockSize); 
	return pRet->pBlock;	
}

bool CMemPool::FreeBlock(char *addr)
{
	MemBlock *block = UsedList.DeletBlockWithAddr(addr);
	if(NULL == block)
	{
		LogI("FreeBlock error: get block null\n");
		return false;
	}
	//init the block
	block->pNext = NULL;
	memset(block->pBlock, 0, BlockSize);

	FreeList.PushTrail(block);
	return true;
}

/*****************not public*************************/
void CMemPool::FreeMem()
{
	ContnBlockInf * pTmp = ContnBlockList, *pConBlockNext;
	while(pTmp != NULL)
	{
		pConBlockNext = pTmp->pNext;

		delete[] pTmp->pMemBlock;
		delete[] pTmp->pMemBlockList;
		delete pTmp;
		
		pTmp = pConBlockNext;
	}
}
ContnBlockInf* CMemPool::GetContnBlock(int blockSize, int blockCount)
{
        if(blockCount <= 0 || blockSize <= 0)
        {
        	return NULL;
        }
        //get block info struct
        ContnBlockInf *pContnBlock =new ContnBlockInf();
        printf("pContnBlock=%x\n", pContnBlock);
        if(NULL == pContnBlock)
        {
        	return NULL;
        }	
        memset(pContnBlock, 0, sizeof(pContnBlock));

        //get mem block
        pContnBlock->pMemBlock = new char[blockSize * blockCount];
        if(NULL == pContnBlock->pMemBlock)
        {
        	delete pContnBlock;
        	return NULL;
        }
        memset(pContnBlock->pMemBlock, 0, sizeof(pContnBlock->pMemBlock));

        //get mem list
        pContnBlock->pMemBlockList = new MemBlock[blockCount];
        if(NULL == pContnBlock->pMemBlockList)
        {
        	delete pContnBlock->pMemBlock;
        	delete pContnBlock;
        	return NULL;
        }
        memset(pContnBlock->pMemBlockList, 0, sizeof(pContnBlock->pMemBlockList));//sizeof(MemBlock) * blockCount

        //init
        for(int i = 0; i < blockCount; i++)
        {
            pContnBlock->pMemBlockList[i].pBlock =(char *)(&(pContnBlock->pMemBlock[i*BlockSize]));
            if(i < blockCount - 1)//it's null when i = blockcount-1 because the init before
            {
        	    pContnBlock->pMemBlockList[i].pNext = &(pContnBlock->pMemBlockList[i+1]);
            }
        }
        printf("pMemBlockList[blockCount - 1].pNext=%x\n", pContnBlock->pMemBlockList[blockCount - 1].pNext);
        pContnBlock->pMemBlockList[blockCount - 1].pNext = NULL;
        //for test
        MemBlock *tmp = pContnBlock->pMemBlockList;
        while(tmp != NULL)
        {
        	LogI("tmp = %d, tmp->pBlock=%d\n", tmp, tmp->pBlock);
        	tmp = tmp->pNext;
        }
        
	return pContnBlock;	
}
bool CMemPool::ExtendPool(int size, int count)
{
	if(GrowCount++ >= GROW_COUNT_MAX)
	{
		LogE("Grow Count max\n");
		return false;
	}
	ContnBlockInf *pcblock = GetContnBlock(size,count);
	ContnBlockInf *ptmp = NULL;
	if(NULL == pcblock)
	{
		LogE("ExtendPool: the continue block is null\n");
		return false;
	}
	//add to the ContnBlockList	
	if(NULL == ContnBlockList)
	{
		ContnBlockList = pcblock;
	}
	else
	{
		ptmp = ContnBlockList;
		while(ptmp != NULL)
		{
			if(ptmp->pNext == NULL)
			{
				ptmp->pNext = pcblock;
                           pcblock->pNext = NULL;
				break;
			}
			ptmp = ptmp->pNext;
		}
	}
       //add to the free list
       printf("pcblock=%x\n", pcblock);
       printf("pcblock->pMemBlockList=%x\n", pcblock->pMemBlockList);
	MemBlock *block = pcblock->pMemBlockList, *tmp = NULL;
	while(block != NULL)
	{
             assert(block);
             printf("the block is %x\n", block);
             printf(" the next is %x\n", block->pNext);
		tmp = block->pNext;
		FreeList.PushTrail(block);
		block = tmp;
	}
	return true;
}
long CMemPool::GetBlockSize()
{
    return BlockSize;
}

