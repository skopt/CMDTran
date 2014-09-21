#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "RecvDataProc.h"


CRecvDataProc::CRecvDataProc()
{
}
CRecvDataProc::~CRecvDataProc()
{
}
void CRecvDataProc::Init()
{
	pthread_mutex_init(&RecvDataMPLock, NULL);
	RecvDataProcTM.InitPool(4);
	RecvDataMP.CreatPool(MEM_POOL_BLOCK_SIZE, 100, 50);
	
}
char* CRecvDataProc::GetBuff()
{
	char *v_pBuff;
	pthread_mutex_lock(&RecvDataMPLock);
	v_pBuff = RecvDataMP.GetBlock();
	pthread_mutex_unlock(&RecvDataMPLock);
	return v_pBuff;
}
bool CRecvDataProc::AddRecvData(int sock, char *pbuff, int len)
{
	RcDataProcTV v_DataTV;
	if(NULL == pbuff)
	{
		return false;
	}
	
	v_DataTV.socket = sock;
	v_DataTV.pData = pbuff;
	v_DataTV.len = len;
	v_DataTV.pthis = this;

	RecvDataProcTM.AddTask(Task<RcDataProcTV>(v_DataTV, RecvDataProcFun));
	printf("add recv data\n");
	return true;
}
void CRecvDataProc::RecvDataProcFun(void *pArgu)
{
	RcDataProcTV *pRcDataProcTV = (RcDataProcTV *)pArgu;
	CRecvDataProc *pthis = (CRecvDataProc *)pRcDataProcTV->pthis;
    printf("to send data\n");
    write(pRcDataProcTV->socket, pRcDataProcTV->pData, pRcDataProcTV->len);
    //free memery
	pthread_mutex_lock(&pthis->RecvDataMPLock);
	if(!pthis->RecvDataMP.FreeBlock(pRcDataProcTV->pData))
	{
		printf("Free block failed!!!");
	}
	pthread_mutex_unlock(&pthis->RecvDataMPLock);
	printf("free mem\n");
}