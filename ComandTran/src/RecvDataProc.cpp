#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "RecvDataProc.h"
#include "Log.h"


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
:RecvFrameProcTM("DataProcThreads")
{
    Init();
}
CRecvDataProc::~CRecvDataProc()
{
}
void CRecvDataProc::Init()
{
    pthread_mutex_init(&RecvFrameMPLock, NULL);
    pthread_mutex_init(&ClientMapLock, NULL);
    RecvFrameProcTM.InitPool(4);
    //RecvFrameProcTM.GetTask_Cust = CRecvDataProc::GetTaskCustImp;
    RecvFrameMP.CreatPool(MEM_POOL_BLOCK_SIZE, 1024*100, 1024*10);
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
    	LogError("Free block failed!!!");
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

    LogInf("new client, socket=%d",sock);
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

    LogInf("socket quit, socket=%d", sock);
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
    		LogError("error code");
    		break;			
    }
}
bool CRecvDataProc::NewClient(int sock, char *pFrame)
{
    if(NULL == pFrame)
    {
    	LogError("bad param");
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
    //ack
    char login_ack_frame[5] = {0x7E, 0x00, 0x05,0x90,0xA5};
    SendData(sock, login_ack_frame, 5, NULL);
    LogInf("new client, type=%d, Camera Id=%d", v_iClientType, it->second.CameraId);
    return true;
}
int CRecvDataProc::SendCallBakcFun(char* buff, int len, int code)
{
    return 0;
}
void CRecvDataProc::PicDataRecv(int sock, char *pFrame, int FrameLen)
{
    if(NULL == pFrame)
    {
    	LogError("bad param");
    	return;
    }
    //get the phone client camera id
    map<int, ClientInfo>::iterator v_clientIt;
    v_clientIt = ClientMap.find(sock);
    if(v_clientIt == ClientMap.end())
    {
    	return;
    }
    //ack
    char ack_frame[7];
    FrameInit(ack_frame, 7, 0x90);
    ack_frame[4] = pFrame[6];
    ack_frame[5] = pFrame[7];
    //SendData(sock, ack_frame, 7, NULL);
    //transfer
    int v_iCameraId = v_clientIt->second.CameraId;

    map<int, ClientInfo>::iterator it;
    for(it = ClientMap.begin(); it != ClientMap.end(); it++)
    {
    	if(it->first != sock && CLIENT_TYPE_PC == it->second.ClientType
    		&& v_iCameraId == it->second.CameraId)
    	{
    		SendData(it->first,pFrame,FrameLen, CRecvDataProc::SendCallBakcFun);
    	}
    }
}
void CRecvDataProc::ResetCameraId(int sock, char *pFrame, int FrameLen)
{
    if(NULL == pFrame)
    {
    	LogError("bad param");
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
    LogInf("socket %d change camera id to %d", sock, it->second.CameraId);
}
void CRecvDataProc::FrameInit(char* frame, int len, unsigned char code)
{
    frame[0] = 0x7E;
    frame[1] = (unsigned char) (len / 256);
    frame[2] = (unsigned char) (len % 256);
    frame[3] = code;
    frame[len - 1] = 0xA5;
}