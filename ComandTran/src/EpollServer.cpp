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


CEpollServer::CEpollServer(int port)
{
   m_iPort = port;
   pthread_mutex_init(&m_SocketInfoLock, NULL);
   pthread_mutex_init(&m_SendDataMemLock, NULL);
   pthread_mutex_init(&m_SendingListLock, NULL);
   pthread_cond_init(&m_SendingListReady, NULL);
   SendDataMem.CreatPool(MEM_POOL_BLOCK_SIZE, 100, 50);
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
	if(!InitEvn())
	{
		printf("init env failed\n");
		return false;
	}

	RecvDataProc.Init((void *) this, CEpollServer::SendDataIntf);

	pthread_t v_ThreadID;	
	int ret = pthread_create(&v_ThreadID, NULL, _EventRecvFun, (void *)this);
	if(0 != ret)
	{
		printf("thread creat failed\n");
		return false;
	}
	ThreadManager.InitPool(4);
       if(0 != pthread_create(&SendProcThread, NULL, _SendEventProcFun, (void *)this))
       {
           printf("send thread creat faild\n");
       }
	return true;
}
bool CEpollServer::SendDataIntf(void * pArgu,int sock,char * buffer,int len)
{
    if(NULL == pArgu)
        return false;

    CEpollServer *pthis = (CEpollServer *) pArgu;
    //printf("send data\n");
    //test
    int v_iTotal = ((unsigned char)buffer[4])*256 + (unsigned char)buffer[5];
    int v_iFrameIndex = ((unsigned char)buffer[6])*256 + (unsigned char)buffer[7];
    printf("*******************************v_iTotal=%d, v_iFrameIndex=%d\n", v_iTotal, v_iFrameIndex);
    return pthis -> SendData(sock, buffer, len);
}

bool CEpollServer::SendData(int sock, char *buffer, int len)
{
    if(NULL == buffer || len <= 0 || len > MEM_POOL_BLOCK_SIZE)
        return false;
    //get memory and copy to
    pthread_mutex_lock(&m_SendDataMemLock);
    char * buf = SendDataMem.GetBlock();
    //char buf[1200];
    printf("buffer got addr %x\n", buf);
    pthread_mutex_unlock(&m_SendDataMemLock);
    if(NULL == buf)
    {        
        printf("get block failed\n");  
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
    it->second.OutputChain.Push(buf, len);
    pthread_mutex_unlock(&(it->second.OutputChainLock));
    
    //send signal to thread to send data of this sock.
   // if(it->second.CurrSendState == SEND_STATE_ALLOW)
    {
        pthread_mutex_lock(&m_SendingListLock);
        SendingList.push_back(sock);
        pthread_mutex_unlock(&m_SendingListLock);
        pthread_cond_signal(&m_SendingListReady);
        printf("send signal\n");
    }  

    return true;    
}

void* CEpollServer::_EventRecvFun(void *pArgu)
{
       CEpollServer *pthis = (CEpollServer *)pArgu;
	int v_NumRecvFD = 0, i = 0;
	if(!pthis->InitScoket())
	{
		printf("init sock failed\n");
		return (void *)0;
	}
	printf("into eventRecvFun, init socket succeed\n");
	//recv
	while(true)
	{
		v_NumRecvFD = epoll_wait(pthis->m_iEpollfd, pthis->m_Events,EPOLL_EVENT_MAX, -1);
		//printf("v_NumRecvFD=%d\n", v_NumRecvFD);
		//pthis->ProcessRecvEnts(v_NumRecvFD);
		for(i = 0;i < v_NumRecvFD; i++)
		{
			TaskVal tmp;
			tmp.event = pthis->m_Events[i];
			tmp.pthis = pArgu;
			pthis->ThreadManager.AddTask(Task<TaskVal>(tmp,CEpollServer::EventProc));
		}
	}
}
void CEpollServer::EventProc(void *pArgu)
{
	TaskVal *val = (TaskVal *)pArgu;
	CEpollServer *pthis = (CEpollServer *)val->pthis;
	pthis->ProcessRecvEnts(val->event);
}

bool CEpollServer::ProcessRecvEnts(epoll_event event)
{
	int connfd = 0;
	sockaddr_in v_ClntAddr;	
	socklen_t addrlen = sizeof(sockaddr_in);
	epoll_event ev;

	if(event.data.fd == m_iListenSock) 
      {
             connfd = accept(m_iListenSock, (struct sockaddr *)&v_ClntAddr,&addrlen);
             if (connfd < 0)
             {
			printf("accept error\n");
		}
		printf("recv a new client\n");
		fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFD, 0)|O_NONBLOCK);
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
              ev.data.fd = connfd;
		if(epoll_ctl(m_iEpollfd, EPOLL_CTL_ADD, connfd, &ev) < 0)
		{
			printf("Add connfd error\n");
			close(connfd);
		}
		else
		{
                    SocketInformation NewSocket;
                    memset(&NewSocket, 0, sizeof(SocketInformation));//sendprocflag to be false;
                    NewSocket.sockId = connfd;
                    pthread_mutex_init(&(NewSocket.OutputChainLock), NULL);
                    pthread_mutex_lock(&m_SocketInfoLock);
                    m_SocketInfo.insert(pair<int, SocketInformation>(connfd, NewSocket));
                    pthread_mutex_unlock(&m_SocketInfoLock);
			RecvDataProc.AddClient(connfd);
		}
	}
	else if(event.events&EPOLLIN)//to recv data
	{
		ProcessRecvData(event);
	}
	else if(event.events&EPOLLOUT)
	{
		printf("-------------------EPOLLOUT-------------\n");
             map<int, SocketInformation>::iterator it;
             pthread_mutex_lock(&m_SocketInfoLock);
             it = m_SocketInfo.find(event.data.fd);
             if(it == m_SocketInfo.end())
             {
                 pthread_mutex_unlock(&m_SocketInfoLock);
                 return false;
             }
             it->second.CurrSendState = SEND_STATE_ALLOW;
             pthread_mutex_unlock(&m_SocketInfoLock);

             //add to list to trigger to send data
             pthread_mutex_lock(&m_SendingListLock);
             SendingList.push_back(event.data.fd);
             pthread_mutex_unlock(&m_SendingListLock);
             pthread_cond_signal(&m_SendingListReady);
             printf("-------------------EPOLLOUT end-------------\n");
	}
	else
	{
		printf("socket %d quit1\n", event.data.fd);
		epoll_ctl(m_iEpollfd, EPOLL_CTL_DEL, event.data.fd, &ev);
		close(event.data.fd);
		RecvDataProc.QuitClient(event.data.fd);
	}

    return true;
}
bool CEpollServer::ProcessRecvData(epoll_event event)
{
    int nread;
    char buff[MEM_POOL_BLOCK_SIZE *2];
    while(1)
    {
        nread = read(event.data.fd, buff, MEM_POOL_BLOCK_SIZE *2);
        if (nread == 0)
        {
    	   printf("socket %d quit2\n", event.data.fd);
              close(event.data.fd);
    	   RecvDataProc.QuitClient(event.data.fd);
              return false;
        } 
        if (nread < 0)
       {
    	  //other proc
    	  return true;
        }    
        RecvDataProc.AddRecvData(event.data.fd, buff, nread);
    }
    return true;
}
bool ProcessSendData(epoll_event event)
{
    
    return true;
}

void* CEpollServer::_SendEventProcFun(void *pArgu)
{
    int i =3;
    CEpollServer *pthis = (CEpollServer *)pArgu;
    //get a sock which have data and can send
    while(true)
    {
        int sock = 0;
        pthread_mutex_lock(& (pthis->m_SendingListLock));
        while(pthis->SendingList.size() <= 0)
        {
            timeval v_Now;
            timespec v_OutTime;
            gettimeofday(&v_Now, NULL);
            v_OutTime.tv_sec = v_Now.tv_sec + 5;
            v_OutTime.tv_nsec = v_Now.tv_usec* 1000;
            pthread_cond_timedwait(& (pthis->m_SendingListReady), & (pthis->m_SendingListLock), &v_OutTime);
        }
        sock = pthis->SendingList.front();
        pthis->SendingList.pop_front();
        pthread_mutex_unlock(& (pthis->m_SendingListLock));

        //printf("get sock\n");
        //get memory chain
        map<int, SocketInformation>::iterator it;
        pthread_mutex_lock(&(pthis->m_SocketInfoLock));
        it = pthis->m_SocketInfo.find(sock);
        if(it == pthis->m_SocketInfo.end())
        {
            pthread_mutex_unlock(&(pthis->m_SocketInfoLock));
            continue;
        }
        if(true == it->second.SendProcFlag)
        {
            pthread_mutex_unlock(&(pthis->m_SocketInfoLock));
            continue;
        }
        else
        {
            it->second.SendProcFlag = true;
        }
        pthread_mutex_unlock(&(pthis->m_SocketInfoLock));

        //send data
        //printf("to send data\n");
        pthread_mutex_lock(&(it->second.OutputChainLock));
        while(it->second.OutputChain.GetSize() > 0)
        {
            MemNode *node = it->second.OutputChain.GetHead();
            if(NULL == node->pBuffer)
                break;
            int ret = send(sock, node->pBuffer + node->begin, node->len - node->begin, MSG_NOSIGNAL);
            if(ret < 0)
                break;
            else if(ret > 0 && ret < (node->len - node->begin))
            {
                node->begin = ret;
                it->second.CurrSendState = SEND_STATE_WAITING;
                break;
            }
            else if(ret > 0 && ret == node->len - node->begin)
            {
                //test
                int v_iTotal = ((unsigned char)node->pBuffer[4])*256 + (unsigned char)node->pBuffer[5];
                int v_iFrameIndex = ((unsigned char)node->pBuffer[6])*256 + (unsigned char)node->pBuffer[7];
                printf("buffer release addr %x\n", node->pBuffer);
                printf("----------------------------v_iTotal=%d, v_iFrameIndex=%d\n", v_iTotal, v_iFrameIndex);
                it->second.OutputChain.DeleteHead();
                //free the buffer to manager
                pthread_mutex_lock(&pthis->m_SendDataMemLock);
                pthis->SendDataMem.FreeBlock(node->pBuffer);
                pthread_mutex_unlock(&pthis->m_SendDataMemLock);                
            }
        }
        pthread_mutex_unlock(&(it->second.OutputChainLock));
        it->second.SendProcFlag = false;//lock?
    }
}

