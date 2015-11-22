#include "MemBlockListManager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "Log.h"


CMemBlockListManager::CMemBlockListManager()
:m_BlockCount(0)
{
    pListHead = NULL;
	pListTrail = NULL;
}
CMemBlockListManager::~CMemBlockListManager()
{
}

bool CMemBlockListManager::PushHead(MemBlock *pBlock)
{	
	if(NULL == pBlock)
	{
		LogError("PushHead: param is null");
		return false;
	}

	if(pListHead == NULL)//the list is null, let head and trail equal pblock
	{
             pListHead = pBlock;
		pListTrail = pBlock;
		pListHead->pNext = NULL;
	}
	else
	{
		pBlock->pNext = pListHead;
		pListHead = pBlock;
	}
	return true;
}
bool CMemBlockListManager::PushTrail(MemBlock *pBlock)
{
	if(NULL == pBlock)
	{
		LogError("PushHead: param is null");
		return false;
	}

	if(pListTrail == NULL)
	{
		pListHead = pBlock;
		pListTrail = pBlock;
	}
	else
	{
		pListTrail->pNext = pBlock;
		pListTrail = pBlock;
	}
       assert(pListTrail);
	pListTrail->pNext = NULL;//reinit
	return true;
}
MemBlock *CMemBlockListManager::GetBlockHead()
{
	MemBlock *pRet = NULL;
	if(NULL == pListHead)
	{
		LogError("getBlockHead,Head is null\n");
             return NULL;
	}

	pRet = pListHead;
	pListHead = pListHead->pNext;

	if(NULL == pListHead)// the list is null, the trai should  be null too
	{
		pListTrail = NULL;
	}

	return pRet;
}

MemBlock* CMemBlockListManager::DeletBlockWithAddr(char *addr)
{
	MemBlock *tmp = NULL, *front = NULL;
	if(NULL == addr || NULL == pListHead)
	{
		LogError("DeletBlockWithAddr: param is null");
		return NULL;
	}
	do
	{
		//check the head
		if(pListHead != NULL && pListHead->pBlock != NULL && pListHead->pBlock == addr)
		{
			tmp = pListHead;
			pListHead = pListHead->pNext;
			break;//tmp is return
		}
		//the others
             front = pListHead;
	      tmp = front->pNext;
		while(tmp != NULL)
		{
			if(tmp->pBlock != NULL && tmp->pBlock == addr)
			{
				 front->pNext = tmp->pNext;
				 break;//tmp is return
			}
			front = tmp;
			tmp = tmp->pNext;
		}
	}while(0);
	if(tmp == pListTrail)
	{
		pListTrail = front;
	}    
	return tmp;
}
bool CMemBlockListManager::IsEmpty()
{
	if(NULL == pListHead)
	{
		return true;
	}
	return false;
}
