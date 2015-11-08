#ifndef _MEM_CHAIN_H_
#define _MEM_CHAIN_H_

#include "MemPool.h"
typedef int (*SendCallBack)(char* buff, int len, int code);

struct MemNode{
    MemNode *pNext;
    char *pBuffer;
    SendCallBack CallBackFun;
    int begin;
    int len;
};


class CMemChain{
public:
    CMemChain();
    ~CMemChain();
    MemNode* GetHead();
    MemNode* GetIndex(int index);
    MemNode Pop();
    bool Push(char* buffer, int len, SendCallBack backfun);
    bool DeleteAt(int index);
    bool DeleteHead();
    int GetSize();
private:
    int m_iSize;

public:

private:
    MemNode* m_pChainHead;
    MemNode* m_pChainEnd;
};
#endif