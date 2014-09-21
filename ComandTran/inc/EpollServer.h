#ifndef EPOLLSERVER_H
#define EPOLLSERVER_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "ThreadPool.h"
#include "RecvDataProc.h"

#define EPOLL_EVENT_MAX 1024

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
	CThreadPool<TaskVal> ThreadManager;
	CRecvDataProc RecvDataProc;

public:
	CEpollServer(int port);
	bool Start();

private:
	bool InitEvn();
	bool InitScoket();
	bool AddLisSockEvent();
	static void* _EventRecvFun(void *pArgu);
	bool ProcessRecvEnts(epoll_event event);
	bool ProcessRecvData(epoll_event event);
	static void EventProc(void *pArgu);
};
#endif
