#ifndef _RECVDATAPROC_
#define _RECVDATAPROC_
#include "MasterWorkPool.h"
#include "MemPool.h"
#include <pthread.h>
#include <map>
#include "FrameRestruct.h"
#include "MemChain.h"
#include "RecvDataProcIntf.h"


//-----------------------define----------------------
#define MEM_POOL_BLOCK_SIZE 1024 + 128
#define CLIENT_TYPE_PHONE 0
#define CLIENT_TYPE_PC  1

typedef bool (* SendDataFun) (void * pArgu, int sock, char *buffer, int len, SendCallBack backfun);

class CRecvDataProc;
class CFrameProcTask:public CTask
{
public:
    CFrameProcTask(int sock, char* frame, int len, CRecvDataProc& dataproc);
    ~CFrameProcTask();
    void ProcessTask();
private:

public:

private:
    int m_Sock;
    char* m_pFrame;
    int m_FrameLen;
    CRecvDataProc& m_DataProc;
};

//client info
struct ClientInfo
{
	int socket;
	int ClientType;  //0:cell phone 1:pc
	int CameraId;  //for cell phone, it is the id; for pc, it the camera id it want ot watch
	CFrameRestruct FrameRestruct;
       int FrameCount;
       bool EnableRead;
};
class CRecvDataProc:public CRecvDataProcIntf
{
public:
    CRecvDataProc();
    ~CRecvDataProc();
    void Init();
    char* GetBuff();
    void AddClient(int sock);
    void QuitClient(int sock);
    bool AddRecvData(int sock, char *pbuff, int len);
    void AddFrameProcTask(int sock, char* frame, int len);
    void FreeBuff(char *pbuff);
    static bool GetTaskCustImp(void *pArgu);
private:
    void CommandProc(int sock, char *pFrame, int FrameLen);	
    bool NewClient(int sock, char *pFrame);	
    void PicDataRecv(int sock, char *pFrame, int FrameLen);
    void ResetCameraId(int sock, char *pFrame, int FrameLen);
    void FrameInit(char* frame, int len, unsigned char code);
    static int SendCallBakcFun(char* buff, int len, int code); 
    int  IncreaseFrameCount(int sock, int count);
    int  DecreaseFrameCount(int sock);
    ClientInfo* GetClientInfo(int sock);

public:
    friend class CFrameProcTask;
    int m_flow_control_high;
    int m_flow_control_low;
private:	
    CMasterWorkPool RecvFrameProcTM;
    CMemPool RecvFrameMP;

    map<int, ClientInfo*> ClientMap;
    pthread_mutex_t ClientMapLock;

    //SendDataFun SendData;     
};
#endif
