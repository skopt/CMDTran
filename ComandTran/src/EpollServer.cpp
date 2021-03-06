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
#include <sys/prctl.h>
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
            SocketInformation* pNewSocket = new SocketInformation();
            pNewSocket->sockId = connfd;
            pthread_mutex_init(&(pNewSocket->OutputChainLock), NULL);
            pthread_mutex_lock(&m_Epoll.m_SocketInfoLock);
            m_Epoll.m_SocketInfo.insert(pair<int, SocketInformation*>(connfd, pNewSocket));
            pthread_mutex_unlock(&m_Epoll.m_SocketInfoLock);
            m_Epoll.RecvDataProc->AddClient(connfd);
        }
    }
    else if(event.events&EPOLLIN)//to recv data
    {
        //printf("----------------EPOLLIN---------------------\n");
        ProcessRecvData(event);
    }
    else if(event.events&EPOLLOUT)
    {
    	      //LogInf("-------------------EPOLLOUT-------------");
             map<int, SocketInformation*>::iterator it;
             pthread_mutex_lock(&m_Epoll.m_SocketInfoLock);
             it = m_Epoll.m_SocketInfo.find(event.data.fd);
             if(it == m_Epoll.m_SocketInfo.end())
             {
                 pthread_mutex_unlock(&m_Epoll.m_SocketInfoLock);
                 return false;
             }
             it->second->CurrSendState = SEND_STATE_ALLOW;
             pthread_mutex_unlock(&m_Epoll.m_SocketInfoLock);

             //add to list to trigger to send data
             pthread_mutex_lock(&m_Epoll.m_SendingListLock);
             CSocketSendTask* pTask = new CSocketSendTask(m_Epoll, event.data.fd);
             m_Epoll.SendThreadManager.AddTask(event.data.fd,pTask);
             pthread_mutex_unlock(&m_Epoll.m_SendingListLock);
             pthread_cond_signal(&m_Epoll.m_SendingListReady);
             //LogInf("-------------------EPOLLOUT end-------------");
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
              m_Epoll.RemoveSocket(event.data.fd);
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
    map<int, SocketInformation*>::iterator it;
    pthread_mutex_lock(&(m_Epoll.m_SocketInfoLock));
    it = m_Epoll.m_SocketInfo.find(m_soket);
    if(it == m_Epoll.m_SocketInfo.end())
    {
        pthread_mutex_unlock(&(m_Epoll.m_SocketInfoLock));
        delete this;
        return;
    }
    pthread_mutex_unlock(&(m_Epoll.m_SocketInfoLock));

    pthread_mutex_lock(&(it->second->OutputChainLock));
    while(it->second->OutputChain.GetSize() > 0)
    {
        MemNode *node = it->second->OutputChain.GetHead();
        if(NULL == node->pBuffer)
            break;
        int ret = send(m_soket, node->pBuffer + node->begin, node->len - node->begin, MSG_NOSIGNAL);
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
            node->begin += ret;
            it->second->CurrSendState = SEND_STATE_WAITING;
            LogInf("do not send all data, set the state to waitting");
            break;
        }
        else if(ret > 0 && ret == node->len - node->begin)
        {
            it->second->OutputChain.DeleteHead();
            if(node->CallBackFun)
                node->CallBackFun(node->pBuffer, node->len, 0);
            //free the buffer to manager
            m_Epoll.SendDataMem.FreeBlock(node->pBuffer);
        }            
    }
    pthread_mutex_unlock(&(it->second->OutputChainLock));
    delete this;
}
CEpollServer::CEpollServer(int port, CRecvDataProcIntf* dataproc)
:ThreadManager("RecvThreads"), SendThreadManager("SendThreads"),EnableReading(true)
{
    m_iPort = port;
    RecvDataProc = dataproc;
    RecvDataProc->SetEpoll(this);
    pthread_mutex_init(&m_SocketInfoLock, NULL);
    pthread_mutex_init(&m_SendingListLock, NULL);
    pthread_cond_init(&m_SendingListReady, NULL);
    SendDataMem.CreatPool(MEM_POOL_BLOCK_SIZE, 1024*100, 1024*10);
}
bool CEpollServer::InitEvn()
{
    struct rlimit rt;    
    rt.rlim_max = rt.rlim_cur = EPOLL_EVENT_MAX;
    if (setrlimit(RLIMIT_NOFILE, &rt) == -1) 
    {
        LogError("setrlimit error");
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
    char * buf = SendDataMem.GetBlock();
    if(NULL == buf)
    {        
        LogError("get block failed");  
        return false;
    }
    memcpy(buf, buffer, len);

    map<int, SocketInformation*>::iterator it;
    pthread_mutex_lock(&m_SocketInfoLock);
    it = m_SocketInfo.find(sock);
    if(it == m_SocketInfo.end())
    {
        pthread_mutex_unlock(&m_SocketInfoLock);
        SendDataMem.FreeBlock(buf);
        return false;
    }
    pthread_mutex_unlock(&m_SocketInfoLock);
    pthread_mutex_lock(&(it->second->OutputChainLock));
    it->second->OutputChain.Push(buf, len, backfun);
    pthread_mutex_unlock(&(it->second->OutputChainLock));

    //send signal to thread to send data of this sock.
    if(it->second->CurrSendState == SEND_STATE_ALLOW)
    {
        pthread_mutex_lock(&m_SendingListLock);
        CSocketSendTask* pTask = new CSocketSendTask(*this, sock);
        SendThreadManager.AddTask(sock,pTask);
        pthread_mutex_unlock(&m_SendingListLock);
        pthread_cond_signal(&m_SendingListReady);
    }  

    return true;    
}
void CEpollServer::RemoveSocket(int sock)
{
    SocketInformation* socket_closed;
    map<int, SocketInformation*>::iterator it;
    pthread_mutex_lock(&m_SocketInfoLock);
    it = m_SocketInfo.find(sock);
    if(it == m_SocketInfo.end())
    {
        pthread_mutex_unlock(&m_SocketInfoLock);
        return ;
    }
    socket_closed = it->second;
    m_SocketInfo.erase(it);
    pthread_mutex_unlock(&m_SocketInfoLock);
    //release source of this socket
    pthread_mutex_lock(&(socket_closed->OutputChainLock));
    while(socket_closed->OutputChain.GetSize() > 0)
    {
        MemNode* node = socket_closed->OutputChain.GetHead();
        if(node->CallBackFun)
                node->CallBackFun(node->pBuffer, node->len, SEND_FAILD);
        //free the buffer to manager
        SendDataMem.FreeBlock(node->pBuffer);
        socket_closed->OutputChain.DeleteHead();
    }
    pthread_mutex_unlock(&(socket_closed->OutputChainLock));

    delete socket_closed;
}
void* CEpollServer::_EventRecvFun(void *pArgu)
{
    CEpollServer *pthis = (CEpollServer *)pArgu;
    prctl(PR_SET_NAME, "epoll wait", NULL, NULL, NULL);
    int v_NumRecvFD = 0, i = 0;
    if(!pthis->InitScoket())
    {
        LogError("init sock failed");
        return (void *)0;
    }
    while(true)
    {
        v_NumRecvFD = epoll_wait(pthis->m_iEpollfd, pthis->m_Events,EPOLL_EVENT_MAX, 10);
        for(i = 0;i < v_NumRecvFD; i++)
        {
        	CSocketRecvTask* tmp = new CSocketRecvTask(*pthis, pthis->m_Events[i]);
        	pthis->ThreadManager.AddTask(pthis->m_Events[i].data.fd, tmp);
        	//tmp->ProcessTask();
        }
    }
}

bool CEpollServer::EnableRead()
{
    if(EnableReading)
        return true;

    bool ret = true;
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

    map<int, SocketInformation*>::iterator it;
    for(it = m_SocketInfo.begin(); it != m_SocketInfo.end();++it)
    {
            SocketInformation* tmp = it->second;
            ev.data.fd = tmp->sockId;
            if(epoll_ctl(m_iEpollfd, EPOLL_CTL_MOD, tmp->sockId, &ev) < 0)
            {
                LogError("Enable Read failded");
                ret = false;
            }
    }
    
    EnableReading = true;
    return ret;
}

bool CEpollServer::DisableRead()
{
    if(!EnableReading)
        return true;
    
    bool ret = true;
    epoll_event ev;
    ev.events = EPOLLOUT | EPOLLET;

    map<int, SocketInformation*>::iterator it;
    for(it = m_SocketInfo.begin(); it != m_SocketInfo.end();++it)
    {
            SocketInformation* tmp = it->second;
            ev.data.fd = tmp->sockId;
            if(epoll_ctl(m_iEpollfd, EPOLL_CTL_MOD, tmp->sockId, &ev) < 0)
            {
                LogError("Enable Read failded");
                ret = false;
            }
    }

    EnableReading = false;
    return ret;
}

bool CEpollServer::ModifyRead(int sock, bool enable)
{   
    bool ret = true;
    pthread_mutex_lock(&m_SocketInfoLock);
    map<int, SocketInformation*>::iterator it;
    it = m_SocketInfo.find(sock);
    if(it == m_SocketInfo.end())
    {
        pthread_mutex_unlock(&m_SocketInfoLock);
        return true;
    }

    if(enable == it->second->EnableRead)
    {
        pthread_mutex_unlock(&m_SocketInfoLock);
        return true;
    }

    it->second->EnableRead = enable;//executed in sequence for one sock, so no problem 
    pthread_mutex_unlock(&m_SocketInfoLock);

    epoll_event ev;
    ev.data.fd = sock;
    if(enable)
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    else
        ev.events = EPOLLOUT | EPOLLET;

    int res = 1;
    if((res = epoll_ctl(m_iEpollfd, EPOLL_CTL_MOD, sock, &ev)) < 0)
    {
        LogError("Enable Read failded");
        ret = false;
    }
    //printf("modify read %d,res=%d,  ret=%d\n", enable, res, ret);
    fflush(stdout);
    return ret;
}

bool CEpollServer::EnableRead(int sock)
{
    LogInf("sock=%d, disable read", sock);
    return ModifyRead(sock, true);
}

bool CEpollServer::DisableRead(int sock)
{
   LogInf("sock=%d, disable read", sock);
   return ModifyRead(sock,false);
}
