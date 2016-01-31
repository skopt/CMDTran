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
    
    int frames = m_DataProc.DecreaseFrameCount(m_Sock);
  
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
:RecvFrameProcTM("DataProcThreads"), m_flow_control_high(0), m_flow_control_low(0)
{
    Init();
}
CRecvDataProc::~CRecvDataProc()
{
}
void CRecvDataProc::Init()
{
    pthread_mutex_init(&ClientMapLock, NULL);
    RecvFrameProcTM.InitPool(4);
    //RecvFrameProcTM.GetTask_Cust = CRecvDataProc::GetTaskCustImp;
    RecvFrameMP.CreatPool(MEM_POOL_BLOCK_SIZE, 1024*300, 1024*10);
    int total = 1024*300;
    m_flow_control_high = 1024*300;
    m_flow_control_low =512;
}
char* CRecvDataProc::GetBuff()
{
    char *v_pBuff;
    v_pBuff = RecvFrameMP.GetBlock();

    return v_pBuff;
}
void CRecvDataProc::FreeBuff(char *pbuff)
{
    //free memery
    if(!RecvFrameMP.FreeBlock(pbuff))
    {
    	LogError("Free block failed!!!");
    }
}
void CRecvDataProc::AddClient(int sock)
{
    ClientInfo* newClient = new ClientInfo();
    newClient->socket = sock;
    newClient->FrameRestruct.pRecvDataProc = (void *) this;
    newClient->ClientType = -1;
    newClient->FrameCount = 0;
    newClient->EnableRead = true;
    pthread_mutex_lock(&ClientMapLock);
    ClientMap.insert(pair<int, ClientInfo*>(sock, newClient));
    pthread_mutex_unlock(&ClientMapLock);

    LogInf("new client, socket=%d",sock);
}

void CRecvDataProc::QuitClient(int sock)
{
    map<int, ClientInfo*>::iterator it;

    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {
        pthread_mutex_unlock(&ClientMapLock);
        return;
    }
    ClientInfo* client = it->second;
    delete client;
    ClientMap.erase(it);
    pthread_mutex_unlock(&ClientMapLock);

    LogInf("socket quit, socket=%d", sock);
}

bool CRecvDataProc::AddRecvData(int sock, char *pbuff, int len)
{
    map<int, ClientInfo*>::iterator it;
    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {
    	pthread_mutex_unlock(&ClientMapLock);
    	return false;
    }
    pthread_mutex_unlock(&ClientMapLock);
    int frame_count = it->second->FrameRestruct.RestructFrame(sock, pbuff, len);	
    IncreaseFrameCount(sock, frame_count);
    //printf("frame count is %d\n", frame_count);
    //if(frame_count > 0)
        //RecvFrameProcTM.AddTaskBatch(sock, it->second->FrameRestruct.TaskList);
    return true;
}


int  CRecvDataProc::IncreaseFrameCount(int sock, int count)
{
    map<int, ClientInfo*>::iterator it;
    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {
    	pthread_mutex_unlock(&ClientMapLock);
    	return false;
    }
    it->second->FrameCount += count;
    
    int ret = it->second->FrameCount;
    if(ret > m_flow_control_high && it->second->EnableRead == true)
    {
        DisableRead(sock);
        it->second->EnableRead = false;
    }
    pthread_mutex_unlock(&ClientMapLock);
    return ret;
}

int  CRecvDataProc::DecreaseFrameCount(int sock)
{
    map<int, ClientInfo*>::iterator it;
    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {
    	pthread_mutex_unlock(&ClientMapLock);
    	return false;
    }
    int ret = --it->second->FrameCount;
    if(ret < m_flow_control_low && it->second->EnableRead == false)
    {
        EnableRead(sock);
        it->second->EnableRead = true;
    }
    pthread_mutex_unlock(&ClientMapLock);

    return ret;
}

void CRecvDataProc::AddFrameProcTask(int sock, char* frame, int len)
{
    CFrameProcTask* tmp = new CFrameProcTask(sock, frame, len, *this);
    RecvFrameProcTM.AddTask(sock, tmp);
    //tmp->ProcessTask();
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
    map<int, ClientInfo*>::iterator it;
    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {		
    	pthread_mutex_unlock(&ClientMapLock);
    	return false;
    }
    it->second->ClientType = v_iClientType;
    it->second->CameraId = ((unsigned char)pFrame[5] * 256) + (unsigned char)pFrame[6];
    pthread_mutex_unlock(&ClientMapLock);
    //ack
    char login_ack_frame[5] = {0x7E, 0x00, 0x05,0x90,0xA5};
    SendData(sock, login_ack_frame, 5, NULL);
    LogInf("new client, type=%d, Camera Id=%d", v_iClientType, it->second->CameraId);
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
    map<int, ClientInfo*>::iterator v_clientIt;
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
    int v_iCameraId = v_clientIt->second->CameraId;

    map<int, ClientInfo*>::iterator it;
    for(it = ClientMap.begin(); it != ClientMap.end(); it++)
    {
    	if(it->first != sock && CLIENT_TYPE_PC == it->second->ClientType
    		&& v_iCameraId == it->second->CameraId)
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
    map<int, ClientInfo*>::iterator it;
    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {		
    	pthread_mutex_unlock(&ClientMapLock);
    	return;
    }
    it->second->CameraId = ((unsigned char)pFrame[4] * 256) + (unsigned char)pFrame[5];
    pthread_mutex_unlock(&ClientMapLock);
    LogInf("socket %d change camera id to %d", sock, it->second->CameraId);
}
void CRecvDataProc::FrameInit(char* frame, int len, unsigned char code)
{
    frame[0] = 0x7E;
    frame[1] = (unsigned char) (len / 256);
    frame[2] = (unsigned char) (len % 256);
    frame[3] = code;
    frame[len - 1] = 0xA5;
}

ClientInfo* CRecvDataProc::GetClientInfo(int sock)
{
    map<int, ClientInfo*>::iterator it;
    pthread_mutex_lock(&ClientMapLock);
    it = ClientMap.find(sock);
    if(it == ClientMap.end())
    {		
    	pthread_mutex_unlock(&ClientMapLock);
    	return NULL;
    }
    pthread_mutex_unlock(&ClientMapLock);
    return it->second;
}
