#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <map>
#include "ThreadPool.h"
#include "RecvDataProc.h"
#include "MemChain.h"
#include "MemPool.h"

#define EPOLL_EVENT_MAX 1024
#define MEM_POOL_BLOCK_SIZE 1024 + 128
#define SEND_STATE_ALLOW 0
#define SEND_STATE_WAITING 1

struct SocketInformation{
    int sockId;
    CMemChain OutputChain; 
    pthread_mutex_t OutputChainLock;
    bool SendProcFlag;
    int CurrSendState;
};

struct TaskVal
{
	epoll_event event;
	void *pthis;
};

class CEpollServer
{
public:
	

private:
	int m_iListenSock;
	int m_iPort;
	int m_iEpollfd;
	epoll_event m_ListenEvent;
	epoll_event m_Events[EPOLL_EVENT_MAX];
       map<int, SocketInformation> m_SocketInfo;
       pthread_mutex_t m_SocketInfoLock;
	CThreadPool<TaskVal> ThreadManager;
	CRecvDataProc RecvDataProc;
       list<int> SendingList;
       pthread_mutex_t m_SendingListLock;
       pthread_cond_t m_SendingListReady;
       pthread_t SendProcThread;
       //memory about
       CMemPool SendDataMem;
       pthread_mutex_t m_SendDataMemLock;
       

public:
	CEpollServer(int port);
	bool Start();

private:
	bool InitEvn();
	bool InitScoket();
	bool AddLisSockEvent();
	static void* _EventRecvFun(void *pArgu);
       static void* _SendEventProcFun(void *pArgu);
	bool ProcessRecvEnts(epoll_event event);
	bool ProcessRecvData(epoll_event event);
       bool ProcessSendData(epoll_event event);
       bool SendData(int sock, char *buffer, int len);
       static bool SendDataIntf(void *pArgu, int sock, char *buffer, int len);
	static void EventProc(void *pArgu);
       
};
#endif
