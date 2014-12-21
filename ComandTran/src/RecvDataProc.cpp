#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "RecvDataProc.h"

#define LogI printf

CRecvDataProc::CRecvDataProc()
{
}
CRecvDataProc::~CRecvDataProc()
{
}
void CRecvDataProc::Init()
{
	pthread_mutex_init(&RecvFrameMPLock, NULL);
	pthread_mutex_init(&ClientMapLock, NULL);
	RecvFrameProcTM.InitPool(4);
	RecvFrameMP.CreatPool(MEM_POOL_BLOCK_SIZE, 100, 50);
	
}
char* CRecvDataProc::GetBuff()
{
	char *v_pBuff;
	pthread_mutex_lock(&RecvFrameMPLock);
	v_pBuff = RecvFrameMP.GetBlock();
	pthread_mutex_unlock(&RecvFrameMPLock);
	return v_pBuff;
}
void CRecvDataProc::FreeBuff(char *pbuff)
{
	//free memery
	pthread_mutex_lock(&RecvFrameMPLock);
	if(!RecvFrameMP.FreeBlock(pbuff))
	{
		printf("Free block failed!!!");
	}
	pthread_mutex_unlock(&RecvFrameMPLock);
}
void CRecvDataProc::AddClient(int sock)
{
	char *pbuffer = new char[MEM_POOL_BLOCK_SIZE * 2];
	if(NULL == pbuffer)
		return;
	ClientInfo newClient;
	newClient.socket = sock;
	newClient.FrameRestruct.pRecvDataProc = (void *) this;
	newClient.ClientType = -1;
	pthread_mutex_lock(&ClientMapLock);
    ClientMap.insert(pair<int, ClientInfo>(sock, newClient));
	pthread_mutex_unlock(&ClientMapLock);

	LogI("new client, socket=%d\n",sock);
}

void CRecvDataProc::QuitClient(int sock)
{
	map<int, ClientInfo>::iterator it;

    pthread_mutex_lock(&ClientMapLock);
	it = ClientMap.find(sock);
	if(it == ClientMap.end())
	{
		pthread_mutex_unlock(&ClientMapLock);
		return;
	}
	ClientMap.erase(it);
	pthread_mutex_unlock(&ClientMapLock);

	LogI("socket quit, socket=%d\n", sock);
}

bool CRecvDataProc::AddRecvData(int sock, char *pbuff, int len)
{
	map<int, ClientInfo>::iterator it;
	
    pthread_mutex_lock(&ClientMapLock);
	it = ClientMap.find(sock);
	if(it == ClientMap.end())
	{		
		pthread_mutex_unlock(&ClientMapLock);
		return false;
	}
	it->second.FrameRestruct.RestructFrame(sock, pbuff, len);	
	pthread_mutex_unlock(&ClientMapLock);
	
	printf("add recv data\n");
	return true;
}

void CRecvDataProc::AddFrameProcTask(RecvFrameProcTV task)
{
	task.pthis = this;
	RecvFrameProcTM.AddTask(Task<RecvFrameProcTV>(task,
		RecvDataProcFun));
}
void CRecvDataProc::RecvDataProcFun(void *pArgu)
{
	RecvFrameProcTV *pRcDataProcTV = (RecvFrameProcTV *)pArgu;
	CRecvDataProc *pthis = (CRecvDataProc *)pRcDataProcTV->pthis;
	if(NULL == pRcDataProcTV || NULL == pRcDataProcTV->pFrame || NULL == pthis)
	{
		printf("bad param\n");
		return;
	}
	pthis->CommandProc(pRcDataProcTV->RecvSocket, pRcDataProcTV->pFrame, pRcDataProcTV->FameLen);
    //write(pRcDataProcTV->RecvSocket, pRcDataProcTV->pFrame, pRcDataProcTV->FameLen);
    

    pthis->FreeBuff(pRcDataProcTV->pFrame);
}
void CRecvDataProc::CommandProc(int sock, char *pFrame, int FrameLen)
{
	unsigned char code = (unsigned char)pFrame[3];
	switch(code)
	{
		case 0x00:
			if(!NewClient(sock,pFrame))
			{
				QuitClient(sock);
				close(sock);
			}
			break;
		case 0x10:
			PicDataRecv(sock,pFrame,FrameLen);
			break;
		default:
			printf("error code\n");
			break;			
	}
}
bool CRecvDataProc::NewClient(int sock, char *pFrame)
{
	if(NULL == pFrame)
	{
		printf("bad param\n");
		return false;
	}
	//get client type
	int v_iClientType;
	if(CLIENT_TYPE_PHONE == pFrame[4] || CLIENT_TYPE_PC == pFrame[4])
	{
		v_iClientType = pFrame[4];
	}
	else
	{
		return false;
	}
	map<int, ClientInfo>::iterator it;
	pthread_mutex_lock(&ClientMapLock);
	it = ClientMap.find(sock);
	if(it == ClientMap.end())
	{		
		pthread_mutex_unlock(&ClientMapLock);
		return false;
	}
	it->second.ClientType = v_iClientType;
	pthread_mutex_unlock(&ClientMapLock);
	//response
	//char v_cResponse[6] = {0x7E,0x00,0x06,0x00,0x00,0xA5};
	//write(sock, v_cResponse, 6);
	printf("new client, type=%d\n", v_iClientType);
	return true;
}
void CRecvDataProc::PicDataRecv(int sock, char *pFrame, int FrameLen)
{
	if(NULL == pFrame)
	{
		printf("bad param\n");
		return;
	}
	map<int, ClientInfo>::iterator it;
	for(it = ClientMap.begin(); it != ClientMap.end(); it++)
	{
		if(it->first != sock && CLIENT_TYPE_PC == it->second.ClientType)
		{
			write(it->first,pFrame,FrameLen);
		}
	}
}