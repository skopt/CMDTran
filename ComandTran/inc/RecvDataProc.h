#ifndef _RECVDATAPROC_
#define _RECVDATAPROC_
#include "ThreadPool.h"
#include "MemPool.h"
#include <pthread.h>

//-----------------------define----------------------
#define MEM_POOL_BLOCK_SIZE 1024


struct RcDataProcTV
{
	int socket;
	char *pData;
	int len;
	void *pthis;
};
class CRecvDataProc 
{
public:
	CRecvDataProc();
	~CRecvDataProc();
	void Init();
	char* GetBuff();
	bool AddRecvData(int sock, char *pbuff, int len);
private:
	static void RecvDataProcFun(void *pArgu);
	
public:

private:	
	CThreadPool<RcDataProcTV> RecvDataProcTM;
	CMemPool RecvDataMP;
	pthread_mutex_t RecvDataMPLock;
};
#endif
