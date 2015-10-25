#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "RecvDataProc.h"

#define LogI printf

CFrameProcTask::CFrameProcTask(int sock, char* frame, int len, CRecvDataProc& dataproc)
:m_Sock(sock),m_pFrame(frame), m_FrameLen(len), m_DataProc(dataproc)
{
}
CFrameProcTask::~CFrameProcTask()
{
}

void CFrameProcTask::ProcessTask()
{
    	m_DataProc.CommandProc(m_Sock, m_pFrame, m_FrameLen);
	m_DataProc.FreeBuff(m_pFrame);
       delete this;
	/* struct changed, do not need the flag anymore
       map<int, ClientInfo>::iterator it;
	pthread_mutex_lock(&m_DataProc.ClientMapLock);
	it = m_DataProc.ClientMap.find(m_Sock);
	if(it == m_DataProc.ClientMap.end())
	{		
		pthread_mutex_unlock(&m_DataProc.ClientMapLock);
		return;
	}
	it->second.FrameProcessingFlag = false;
	pthread_mutex_unlock(&pthis->ClientMapLock);      
	*/
}

CRecvDataProc::CRecvDataProc()
{
}
CRecvDataProc::~CRecvDataProc()
{
}
void CRecvDataProc::Init(void *pArgu, SendDataFun sendfun)
{
       if(NULL != sendfun && NULL != pArgu)
       {
         SendData = sendfun;
         pEpollServer = pArgu;
       }
	pthread_mutex_init(&RecvFrameMPLock, NULL);
	pthread_mutex_init(&ClientMapLock, NULL);
	RecvFrameProcTM.InitPool(4);
	//RecvFrameProcTM.GetTask_Cust = CRecvDataProc::GetTaskCustImp;
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
		printf("Free block failed!!!\n");
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
	pthread_mutex_unlock(&ClientMapLock);
	it->second.FrameRestruct.RestructFrame(sock, pbuff, len);	
	return true;
}

void CRecvDataProc::AddFrameProcTask(int sock, char* frame, int len)
{
	CFrameProcTask* tmp = new CFrameProcTask(sock, frame, len, *this);
	RecvFrameProcTM.AddTask(sock, tmp);
}

/*struct changed , do not need anymore
bool CRecvDataProc::GetTaskCustImp(void *pArgu)
{
	bool ret = false;
	if(NULL == pArgu)
	{
		return ret;
	}
	RecvFrameProcTV * pRcDataProcTV = (RecvFrameProcTV *)pArgu;
	CRecvDataProc *pthis = (CRecvDataProc *)pRcDataProcTV->pthis;
	map<int, ClientInfo>::iterator it;
	pthread_mutex_lock(&pthis->ClientMapLock);
	it = pthis->ClientMap.find(pRcDataProcTV->RecvSocket);
	if(it == pthis->ClientMap.end())
	{		
		pthread_mutex_unlock(&pthis->ClientMapLock);
		return false;
	}
	if(false == it->second.FrameProcessingFlag)
	{
		ret = true;
		it->second.FrameProcessingFlag = true;
	}
	pthread_mutex_unlock(&pthis->ClientMapLock);
	return ret;
}*/
void CRecvDataProc::CommandProc(int sock, char *pFrame, int FrameLen)
{
	unsigned char code = (unsigned char)pFrame[3];
       //printf("receieve a frame code = %d\n", code);
	switch(code)
	{
		case 0x00:
			if(!NewClient(sock,pFrame))
			{
				QuitClient(sock);
				close(sock);
			}
			break;
		case 0x01:
                    ResetCameraId(sock,pFrame,FrameLen);
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
	it->second.CameraId = ((unsigned char)pFrame[5] * 256) + (unsigned char)pFrame[6];
	pthread_mutex_unlock(&ClientMapLock);
	//response
	//char v_cResponse[6] = {0x7E,0x00,0x06,0x00,0x00,0xA5};
	//write(sock, v_cResponse, 6);
	printf("new client, type=%d, Camera Id=%d\n", v_iClientType, it->second.CameraId);
	return true;
}
void CRecvDataProc::PicDataRecv(int sock, char *pFrame, int FrameLen)
{
	if(NULL == pFrame)
	{
		printf("bad param\n");
		return;
	}
	//get the phone client camera id
	map<int, ClientInfo>::iterator v_clientIt;
	v_clientIt = ClientMap.find(sock);
	if(v_clientIt == ClientMap.end())
	{
		return;
	}
	int v_iCameraId = v_clientIt->second.CameraId;

	map<int, ClientInfo>::iterator it;
	//test
	int v_iTotal = ((unsigned char)pFrame[4])*256 + (unsigned char)pFrame[5];
       int v_iFrameIndex = ((unsigned char)pFrame[6])*256 + (unsigned char)pFrame[7];
	//printf("*******************************v_iTotal=%d, v_iFrameIndex=%d\n", v_iTotal, v_iFrameIndex);
	//test
	for(it = ClientMap.begin(); it != ClientMap.end(); it++)
	{
		if(it->first != sock && CLIENT_TYPE_PC == it->second.ClientType
			&& v_iCameraId == it->second.CameraId)
		{
			SendData(pEpollServer, it->first,pFrame,FrameLen);
			//write(it->first,pFrame,FrameLen);
		}
	}
}
void CRecvDataProc::ResetCameraId(int sock, char *pFrame, int FrameLen)
{
	if(NULL == pFrame)
	{
		printf("bad param\n");
		return;
	}
	map<int, ClientInfo>::iterator it;
	pthread_mutex_lock(&ClientMapLock);
	it = ClientMap.find(sock);
	if(it == ClientMap.end())
	{		
		pthread_mutex_unlock(&ClientMapLock);
		return;
	}
	it->second.CameraId = ((unsigned char)pFrame[4] * 256) + (unsigned char)pFrame[5];
	pthread_mutex_unlock(&ClientMapLock);
	printf("socket %d change camera id to %d\n", sock, it->second.CameraId);
}