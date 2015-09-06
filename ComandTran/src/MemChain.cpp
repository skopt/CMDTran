#include "MemChain.h"
#include <cstring>
#include <stdio.h>

CMemChain::CMemChain()
{
    m_pChainHead = NULL;
    m_pChainEnd = NULL;
    m_iSize = 0;
}
CMemChain::~CMemChain()
{
    MemNode *ptmp = NULL;
    while(m_pChainHead != NULL)
    {
        ptmp = m_pChainHead->pNext;
        delete m_pChainHead;
        m_pChainHead = ptmp;
    }
}

bool CMemChain::Push(char * buffer,int len)
{
    if(NULL == buffer)
        return false;

    MemNode *pNew = new MemNode();
    memset(pNew, 0, sizeof(MemNode));

    pNew->pBuffer = buffer;
    pNew->len = len;
    pNew->begin = 0;
    
    if(m_pChainHead == NULL && m_pChainEnd == NULL)
    {
        m_pChainHead = m_pChainEnd = pNew;        
    }
    else
    {
        m_pChainEnd->pNext = pNew;
        m_pChainEnd = pNew;
    }
    m_iSize++;
        
    return true;
}
MemNode* CMemChain::GetIndex(int index)
{
    if(index < 0)
        return NULL;
    MemNode * node = m_pChainHead;
    int pos = 0;
    while(pos < index && node != NULL)
    {
        node = node->pNext;
        pos++;
    }

    if(NULL == node)
        return NULL;
    else
        return node;
}
MemNode* CMemChain::GetHead()
{
    MemNode *tmp = GetIndex(0);
    printf("CMemChain: get addr %x\n", tmp->pBuffer);
    return tmp;
}
bool CMemChain::DeleteAt(int index)
{
    if(index < 0|| NULL == m_pChainHead)
        return false;

    if(m_pChainHead == m_pChainEnd)//only one node
    {
        delete m_pChainHead;
        m_pChainHead = m_pChainEnd = NULL;
        m_iSize--;
        return true;
    }

    MemNode *ptmp = m_pChainHead->pNext, *pPreHead = m_pChainHead;
    int pos = 1;
    while(pos < index && ptmp != NULL)
    {
         pPreHead = ptmp;
         ptmp = ptmp->pNext;
         pos++;
    }
    if(NULL == ptmp)
        return false;
    else
    {
        if(ptmp == m_pChainEnd)
            m_pChainEnd = pPreHead;
        pPreHead->pNext = ptmp->pNext;
        delete ptmp;
        m_iSize--;
        return true;
    }
    
}
bool CMemChain::DeleteHead()
{
    //return DeleteAt(0);    
     if(NULL == m_pChainHead)
        return false;

    if(m_pChainHead == m_pChainEnd)//only one node
    {
        delete m_pChainHead;
        m_pChainHead = m_pChainEnd = NULL;
        m_iSize--;
        return true;
    }

     MemNode *ptmp = m_pChainHead;
     m_pChainHead = m_pChainHead->pNext;
     m_iSize--;
     delete ptmp;
     return true;
}
MemNode CMemChain::Pop()
{
    MemNode *ret = GetHead();
    DeleteAt(0);
    return *ret;
}
int CMemChain::GetSize()
{
    return m_iSize;
}