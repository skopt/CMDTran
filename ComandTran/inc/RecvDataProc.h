#ifndef _RECVDATAPROC_
#define _RECVDATAPROC_
#include "ThreadPool.h"
#include "MemPool.h"
#include <pthread.h>
#include <map>
#include "FrameRestruct.h"

//-----------------------define----------------------
#define MEM_POOL_BLOCK_SIZE 1024
#define CLIENT_TYPE_PHONE 0
#define CLIENT_TYPE_PC  1

//client info
struct ClientInfo
{
	int socket;
	CFrameRestruct FrameRestruct;
	int ClientType;  //0:cell phone 1:pc
};
class CRecvDataProc 
{
public:
	CRecvDataProc();
	~CRecvDataProc();
	void Init();
	char* GetBuff();
	void AddClient(int sock);
	void QuitClient(int sock);
	bool AddRecvData(int sock, char *pbuff, int len);
	void AddFrameProcTask(RecvFrameProcTV task);
private:
	static void RecvDataProcFun(void *pArgu);
	void FreeBuff(char *pbuff);
	void CommandProc(int sock, char *pFrame, int FrameLen);
	bool NewClient(int sock, char *pFrame);	
	void PicDataRecv(int sock, char *pFrame, int FrameLen);
public:

private:	
	CThreadPool<RecvFrameProcTV> RecvFrameProcTM;
	CMemPool RecvFrameMP;
	pthread_mutex_t RecvFrameMPLock;
	map<int, ClientInfo> ClientMap;
	pthread_mutex_t ClientMapLock;
};
#endif
