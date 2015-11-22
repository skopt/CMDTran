#include "EpollServer.h"
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include "Log.h"

CSocketRecvTask::CSocketRecvTask(CEpollServer& epoll, epoll_event event)
:m_Epoll(epoll),m_Event(event)
{
}

CSocketRecvTask::~CSocketRecvTask()
{
}

void CSocketRecvTask::ProcessTask()
{
    ProcessRecvEnts(m_Event);
    delete this;
}
bool CSocketRecvTask::ProcessRecvEnts(epoll_event event)
{
	int connfd = 0;
	sockaddr_in v_ClntAddr;	
	socklen_t addrlen = sizeof(sockaddr_in);
	epoll_event ev;

	if(event.data.fd == m_Epoll.GetListenSocket()) 
      {
             connfd = accept(m_Epoll.GetListenSocket(), (struct sockaddr *)&v_ClntAddr,&addrlen);
             if (connfd < 0)
             {
			LogError("accept error");
		}
		LogInf("recv a new client");
		fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFD, 0)|O_NONBLOCK);
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
             ev.data.fd = connfd;
		if(epoll_ctl(m_Epoll.m_iEpollfd, EPOLL_CTL_ADD, connfd, &ev) < 0)
		{
			LogError("Add connfd error");
			close(connfd);
		}
		else
		{
                    SocketInformation NewSocket;
                    memset(&NewSocket, 0, sizeof(SocketInformation));//sendprocflag to be false;
                    NewSocket.sockId = connfd;
                    pthread_mutex_init(&(NewSocket.OutputChainLock), NULL);
                    pthread_mutex_lock(&m_Epoll.m_SocketInfoLock);
                    m_Epoll.m_SocketInfo.insert(pair<int, SocketInformation>(connfd, NewSocket));
                    pthread_mutex_unlock(&m_Epoll.m_SocketInfoLock);
			m_Epoll.RecvDataProc->AddClient(connfd);
		}
	}
	else if(event.events&EPOLLIN)//to recv data
	{
		ProcessRecvData(event);
	}
	else if(event.events&EPOLLOUT)
	{
		LogInf("-------------------EPOLLOUT-------------");
             map<int, SocketInformation>::iterator it;
             pthread_mutex_lock(&m_Epoll.m_SocketInfoLock);
             it = m_Epoll.m_SocketInfo.find(event.data.fd);
             if(it == m_Epoll.m_SocketInfo.end())
             {
                 pthread_mutex_unlock(&m_Epoll.m_SocketInfoLock);
                 return false;
             }
             it->second.CurrSendState = SEND_STATE_ALLOW;
             pthread_mutex_unlock(&m_Epoll.m_SocketInfoLock);

             //add to list to trigger to send data
             pthread_mutex_lock(&m_Epoll.m_SendingListLock);
             CSocketSendTask* pTask = new CSocketSendTask(m_Epoll, event.data.fd);
             m_Epoll.SendThreadManager.AddTask(event.data.fd,pTask);
             pthread_mutex_unlock(&m_Epoll.m_SendingListLock);
             pthread_cond_signal(&m_Epoll.m_SendingListReady);
             LogInf("-------------------EPOLLOUT end-------------");
	}
	else
	{
		LogInf("socket %d quit1", event.data.fd);
		epoll_ctl(m_Epoll.m_iEpollfd, EPOLL_CTL_DEL, event.data.fd, &ev);
		close(event.data.fd);
		m_Epoll.RecvDataProc->QuitClient(event.data.fd);
	}

    return true;
}
bool CSocketRecvTask::ProcessRecvData(epoll_event event)
{
    int nread;
    char buff[MEM_POOL_BLOCK_SIZE *2];
    while(1)
    {
        nread = read(event.data.fd, buff, MEM_POOL_BLOCK_SIZE *2);
        if (nread == 0)
        {
    	       LogError("socket %d quit2", event.data.fd);
              close(event.data.fd);
    	       m_Epoll.RecvDataProc->QuitClient(event.data.fd);
              return false;
        } 
        if (nread < 0)
       {
    	      //other proc
    	      return true;
        }    
        m_Epoll.RecvDataProc->AddRecvData(event.data.fd, buff, nread);
    }
    return true;
}

CSocketSendTask::CSocketSendTask(CEpollServer& epoll, int sock)
:m_Epoll(epoll), m_soket(sock)
{
}
CSocketSendTask::~ CSocketSendTask()
{
}
void CSocketSendTask::ProcessTask()
{
    map<int, SocketInformation>::iterator it;
    pthread_mutex_lock(&(m_Epoll.m_SocketInfoLock));
    it = m_Epoll.m_SocketInfo.find(m_soket);
    if(it == m_Epoll.m_SocketInfo.end())
    {
        pthread_mutex_unlock(&(m_Epoll.m_SocketInfoLock));
        delete this;
        return;
    }
    pthread_mutex_unlock(&(m_Epoll.m_SocketInfoLock));

    pthread_mutex_lock(&(it->second.OutputChainLock));
    LogInf("the count to process is %d", it->second.OutputChain.GetSize() );
    while(it->second.OutputChain.GetSize() > 0)
    {
        MemNode *node = it->second.OutputChain.GetHead();
        if(NULL == node->pBuffer)
            break;
        int ret = send(m_soket, node->pBuffer + node->begin, node->len - node->begin, MSG_NOSIGNAL);
        LogInf("buffer len=%d, send len=%d", node->len, ret);
        if(ret < 0)
        {
                /*
                it->second.OutputChain.DeleteHead();
                pthread_mutex_lock(&pthis->m_SendDataMemLock);
                pthis->SendDataMem.FreeBlock(node->pBuffer);
                pthread_mutex_unlock(&pthis->m_SendDataMemLock);
                printf("chain rest count %d\n", it->second.OutputChain.GetSize());
                it->second.CurrSendState = SEND_STATE_WAITING;
                printf("send data failed, set the state to waitting\n");
                */
            break;
        }
        else if(ret > 0 && ret < (node->len - node->begin))
        {
            node->begin = ret;
            it->second.CurrSendState = SEND_STATE_WAITING;
            LogInf("do not send all data, set the state to waitting");
            break;
        }
        else if(ret > 0 && ret == node->len - node->begin)
        {
            //test
            //int v_iTotal = ((unsigned char)node->pBuffer[4])*256 + (unsigned char)node->pBuffer[5];
            //int v_iFrameIndex = ((unsigned char)node->pBuffer[6])*256 + (unsigned char)node->pBuffer[7];
            //printf("buffer release addr %x\n", node->pBuffer);
            //printf("----------------------------v_iTotal=%d, v_iFrameIndex=%d\n", v_iTotal, v_iFrameIndex);
            it->second.OutputChain.DeleteHead();
            if(node->CallBackFun)
                node->CallBackFun(node->pBuffer, node->len, 0);
            //free the buffer to manager
            pthread_mutex_lock(&(m_Epoll.m_SendDataMemLock));
            m_Epoll.SendDataMem.FreeBlock(node->pBuffer);
            pthread_mutex_unlock(&(m_Epoll.m_SendDataMemLock));
            LogInf("chain rest count %d", it->second.OutputChain.GetSize());
        }            
    }
    pthread_mutex_unlock(&(it->second.OutputChainLock));
    delete this;
}
CEpollServer::CEpollServer(int port, CRecvDataProcIntf* dataproc)
{
   m_iPort = port;
   RecvDataProc = dataproc;
   RecvDataProc->SetEpoll(this);
   pthread_mutex_init(&m_SocketInfoLock, NULL);
   pthread_mutex_init(&m_SendDataMemLock, NULL);
   pthread_mutex_init(&m_SendingListLock, NULL);
   pthread_cond_init(&m_SendingListReady, NULL);
   SendDataMem.CreatPool(MEM_POOL_BLOCK_SIZE, 1024, 100);
}
bool CEpollServer::InitEvn()
{
	struct rlimit rt;    
    rt.rlim_max = rt.rlim_cur = EPOLL_EVENT_MAX;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) 
    {
        perror("setrlimit error");
        return false;
    }
	return true;
}
int CEpollServer::GetListenSocket()
{
    return m_iListenSock;
}

bool CEpollServer::InitScoket()
{
	sockaddr_in v_SerAddr;
	memset(&v_SerAddr, 0, sizeof(sockaddr_in));
	v_SerAddr.sin_family = AF_INET;
	v_SerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	v_SerAddr.sin_port = htons(m_iPort);
	do
	{
		//init socket
		if((m_iListenSock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			break;
		}
		//set socket
		int opt = 1;
		setsockopt(m_iListenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
		if(fcntl(m_iListenSock, F_SETFL, fcntl(m_iListenSock, F_GETFD, 0)|O_NONBLOCK) == -1)
		{
			break;
		}
		//bind    
		if( bind(m_iListenSock, (struct sockaddr*)&v_SerAddr, sizeof(v_SerAddr)) == -1)
		{
		    break;
		}
		//listen 
		if(listen(m_iListenSock, 1024) == -1)
		{
			break;
		}
		if(false == AddLisSockEvent())
		{
			break;
		}
		return true;
	}while(0);
	close(m_iListenSock);
	return false;
}

bool CEpollServer::AddLisSockEvent()
{
	m_iEpollfd = epoll_create(EPOLL_EVENT_MAX);
	m_ListenEvent.events = EPOLLIN|EPOLLET;
	m_ListenEvent.data.fd = m_iListenSock;
	if(epoll_ctl(m_iEpollfd, EPOLL_CTL_ADD, m_iListenSock, &m_ListenEvent) < 0)
	{
		return false;
	}
	return true;
}

bool CEpollServer::Start()
{
      LogInf("stair");
	if(!InitEvn())
	{
		LogError("init env failed");
		return false;
	}

	pthread_t v_ThreadID;	
	int ret = pthread_create(&v_ThreadID, NULL, _EventRecvFun, (void *)this);
	if(0 != ret)
	{
		LogError("thread creat failed");
		return false;
	}
	ThreadManager.InitPool(4);//recv threads 
       LogInf("init recv thread");
       SendThreadManager.InitPool(4);//send threads
       LogInf("init send thread");
       
	return true;
}

bool CEpollServer::SendData(int sock, char *buffer, int len, SendCallBack backfun)
{
    if(NULL == buffer || len <= 0 || len > MEM_POOL_BLOCK_SIZE)
        return false;
    //get memory and copy to
    pthread_mutex_lock(&m_SendDataMemLock);
    char * buf = SendDataMem.GetBlock();
    pthread_mutex_unlock(&m_SendDataMemLock);
    if(NULL == buf)
    {        
        LogError("get block failed");  
        return false;
    }
    memcpy(buf, buffer, len);
    
    map<int, SocketInformation>::iterator it;
    pthread_mutex_lock(&m_SocketInfoLock);
    it = m_SocketInfo.find(sock);
    if(it == m_SocketInfo.end())
    {
        pthread_mutex_unlock(&m_SocketInfoLock);
        return false;
    }
    pthread_mutex_unlock(&m_SocketInfoLock);
    pthread_mutex_lock(&(it->second.OutputChainLock));
    it->second.OutputChain.Push(buf, len, backfun);
    pthread_mutex_unlock(&(it->second.OutputChainLock));
    
    //send signal to thread to send data of this sock.
    if(it->second.CurrSendState == SEND_STATE_ALLOW)
    {
        pthread_mutex_lock(&m_SendingListLock);
        CSocketSendTask* pTask = new CSocketSendTask(*this, sock);
        SendThreadManager.AddTask(sock,pTask);
        pthread_mutex_unlock(&m_SendingListLock);
        pthread_cond_signal(&m_SendingListReady);
    }  

    return true;    
}

void* CEpollServer::_EventRecvFun(void *pArgu)
{
       CEpollServer *pthis = (CEpollServer *)pArgu;
	int v_NumRecvFD = 0, i = 0;
	if(!pthis->InitScoket())
	{
		LogError("init sock failed");
		return (void *)0;
	}
	while(true)
	{
		v_NumRecvFD = epoll_wait(pthis->m_iEpollfd, pthis->m_Events,EPOLL_EVENT_MAX, -1);
		for(i = 0;i < v_NumRecvFD; i++)
		{
			CSocketRecvTask* tmp = new CSocketRecvTask(*pthis, pthis->m_Events[i]);
			pthis->ThreadManager.AddTask(pthis->m_Events[i].data.fd, tmp);
		}
	}
}

