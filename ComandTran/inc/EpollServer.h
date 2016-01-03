#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <map>
#include "MasterWorkPool.h"
#include "RecvDataProcIntf.h"
#include "MemChain.h"
#include "MemPool.h"

#define EPOLL_EVENT_MAX 1024
#define MEM_POOL_BLOCK_SIZE 1024 + 128
#define SEND_STATE_ALLOW 0
#define SEND_STATE_WAITING 1

class SocketInformation{
public:
    SocketInformation()
        :sockId(0),CurrSendState(0)
    {
    } 
    ~SocketInformation(){}

public:
    int sockId;
    CMemChain OutputChain; 
    pthread_mutex_t OutputChainLock;
    int CurrSendState;
};

class CEpollServer;
class CSocketRecvTask: public CTask
{
public:
    CSocketRecvTask(CEpollServer &epoll, epoll_event event);
    ~CSocketRecvTask();
    void ProcessTask();
private:
    bool ProcessRecvEnts(epoll_event event);
    bool ProcessRecvData(epoll_event event);
public:

private:
    CEpollServer & m_Epoll;
    epoll_event m_Event;
};

class CSocketSendTask:public CTask
{
public:
    CSocketSendTask(CEpollServer &epoll, int sock);
    ~CSocketSendTask();
    void ProcessTask();
private:
    CEpollServer &m_Epoll;
    int m_soket;
};

class CEpollServer
{
public:
	int m_iEpollfd;
       map<int, SocketInformation*> m_SocketInfo;
       pthread_mutex_t m_SocketInfoLock;
private:
	int m_iListenSock;
	int m_iPort;
	epoll_event m_ListenEvent;
	epoll_event m_Events[EPOLL_EVENT_MAX];

       CRecvDataProcIntf* RecvDataProc;
       
	CMasterWorkPool ThreadManager;
       CMasterWorkPool SendThreadManager;
	
       pthread_mutex_t m_SendingListLock;
       pthread_cond_t m_SendingListReady;
       pthread_t SendProcThread;
       //memory about
       CMemPool SendDataMem;
       pthread_mutex_t m_SendDataMemLock;
       

public:
	CEpollServer(int port, CRecvDataProcIntf* dataproc);
	bool Start();
       int GetListenSocket();
       bool SendData(int sock, char *buffer, int len, SendCallBack backfun);

private:
	bool InitEvn();
	bool InitScoket();
	bool AddLisSockEvent();
	static void* _EventRecvFun(void *pArgu);
       static void* _SendEventProcFun(void *pArgu);
	bool ProcessRecvEnts(epoll_event event);
	bool ProcessRecvData(epoll_event event);
       bool ProcessSendData(epoll_event event);
       void RemoveSocket(int sock);
       friend class CSocketRecvTask;
       friend class CSocketSendTask;
       
};
#endif
