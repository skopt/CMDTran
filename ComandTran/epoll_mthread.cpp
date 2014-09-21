#include "config.h"
#include "cs_network.h"
#include <iostream>
#include <sys/socket.h>

#define VERSION_SOLARIS 0

#if VERSION_SOLARIS
 #include <port.h>
#else
 #include <sys/epoll.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cs_data_inspect.h"
#include "cs_heart_thread.h"
#include "cs_gdata.h"
#include "cs_work_thread.h"

#define SERV_PORT  5565 // 服务器端口
#define LISTENQ          128   // listen sock 参数
#define MAX_EPOLL_EVENT_COUNT      500   // 同时监听的 epoll 数
#define MAX_READ_BUF_SIZE    1024 * 8 // 最大读取数据 buf

#define SOCKET_ERROR -1

static int epfd;
static int listenfd;

static pthread_t tids[NETWORK_READ_THREAD_COUNT];
static pthread_mutex_t mutex_thread_read;
static pthread_cond_t cond_thread_read;
static void *thread_read_tasks(void *args);

static st_io_context_task *read_tasks_head = NULL;
static st_io_context_task *read_tasks_tail = NULL;

//////////////////////////////////////////////////////////////////////////////////////////


static void append_read_task(st_context *context)
{
 st_io_context_task *new_task = NULL;

 new_task = new st_io_context_task();
 new_task->context = context;

 pthread_mutex_lock(&mutex_thread_read);

 if(read_tasks_tail == NULL)
 {
  read_tasks_head = new_task;
  read_tasks_tail = new_task;
 }   
 else
 {   
  read_tasks_tail->next= new_task;
  read_tasks_tail = new_task;
 }  

 pthread_cond_broadcast(&cond_thread_read);
 pthread_mutex_unlock(&mutex_thread_read); 
}

void _setnonblocking(int sock)
{
 int opts;
 opts = fcntl(sock, F_GETFL);
 if(opts < 0){
  log("fcntl(sock,GETFL)");
  exit(1);
 }

 opts = opts | O_NONBLOCK;
 if(fcntl(sock, F_SETFL, opts)<0){
  log("fcntl(sock,SETFL,opts)");
  exit(1);
 }
}


void* get_network_event(void *param)
{
 long network_event_id;

 int i, sockfd;

 network_event_id = (long) param;

 log("begin thread get_network_event: %ld", network_event_id);

 st_context *context = NULL;

#if VERSION_SOLARIS
 uint_t nfds;
 port_event_t now_ev, ev, events[MAX_EPOLL_EVENT_COUNT];
#else
 unsigned nfds;
 struct epoll_event now_ev, ev, events[MAX_EPOLL_EVENT_COUNT];
#endif


#if VERSION_SOLARIS
 struct timespec timeout;
 timeout.tv_sec = 0;
 timeout.tv_nsec = 50000000;
#endif

 while(1) 
 {
#if VERSION_SOLARIS
  nfds = MAX_EPOLL_EVENT_COUNT;
  if (port_getn(epfd, events, MAX_EPOLL_EVENT_COUNT, &nfds, &timeout) != 0){

   if (errno != ETIME){
    log("port_getn error");
    return false;
   }
  }

  if (nfds == 0){
   continue;
  }
  else{
   // log("on port_getn: %d", nfds);
  }
#else
  //等待epoll事件的发生
  nfds = epoll_wait(epfd, events, MAX_EPOLL_EVENT_COUNT, 100000);
#endif

  //处理所发生的所有事件
  for(i = 0; i < nfds; i++)
  {
   now_ev = events[i];

#if VERSION_SOLARIS
   context = (st_context *)now_ev.portev_user;
#else
   context = (st_context *)now_ev.data.ptr;
#endif

#if VERSION_SOLARIS
   if (now_ev.portev_source != PORT_SOURCE_FD){
    continue;
   }

   if(now_ev.portev_object == listenfd)
#else
   if(context->fd == listenfd)
#endif
   {
#if VERSION_SOLARIS
    // 重新关联listen fd
    port_associate(epfd, PORT_SOURCE_FD, listenfd, POLLIN, context);
#endif

    //append_read_task(NULL, true);
    int connfd;
    struct sockaddr_in clientaddr = {0};
    socklen_t clilen = sizeof(clientaddr);

    connfd = accept(listenfd, (sockaddr *)&clientaddr, &clilen);

    if(connfd == -1){
     log("connfd == -1 [%d]", errno);
     continue;
    }

    _setnonblocking(connfd);
    int nRecvBuf=128*1024;//设置为32K
    setsockopt(connfd, SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
    //发送缓冲区
    int nSendBuf=128*1024;//设置为32K
    setsockopt(connfd, SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));

    int nNetTimeout=1000;//1秒
    //发送时限
    setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&nNetTimeout, sizeof(int));

    //接收时限
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&nNetTimeout, sizeof(int));

    const char *remote_addr = inet_ntoa(clientaddr.sin_addr);
    int  remote_port = ntohs(clientaddr.sin_port);

    context = query_context(connfd, remote_addr, remote_port);

    mutex_lock(mutex_id_pool.mutex);
    context->id = add_client(context);
    mutex_unlock(mutex_id_pool.mutex);

 //#if IS_DEBUG
 //   log("new obj fd: %d, id: %d context:%8X", connfd, context->id, context);
 //#endif

#if VERSION_SOLARIS
    port_associate(epfd, PORT_SOURCE_FD, connfd, POLLIN, context);
#else
    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = context;
    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
#endif
   }

#if VERSION_SOLARIS
   else if(now_ev.portev_events == POLLIN)
   {
    //log("on: now_ev.portev_events == POLLIN");
    append_read_task(context);
   }
   else{
    log("unknow portev_events: %d", now_ev.portev_events);
   }
#else
   else if(now_ev.events & EPOLLIN)
   {
    append_read_task(context);
   }
   else if(now_ev.events & EPOLLOUT)
   { 
    sockfd = context->fd;

    struct epoll_event ev;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = context;
    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
   }
   else if(now_ev.events & EPOLLHUP)
   {
    log("else if(now_ev.events & EPOLLHUP)");
    del_from_network(context);
   }
   else
   {
    log("[warming]other epoll event: %d", now_ev.events);
   }
#endif
  }
 }

 return NULL;
}

bool start_network()
{
 int i, sockfd;
 int optionVal = 0;

 st_context *context = NULL;

#if VERSION_SOLARIS
 uint_t nfds;
 port_event_t ev;
#else
 unsigned nfds;
 struct epoll_event ev;
#endif

 pthread_mutex_init(&mutex_thread_read, NULL);

 pthread_cond_init(&cond_thread_read, NULL);

 int ret;
 pthread_attr_t tattr;

 /* Initialize with default */
 if(ret = pthread_attr_init(&tattr)){
  perror("Error initializing thread attribute [pthread_attr_init(3C)] ");
  return (-1);
 }

 /* Make it a bound thread */
 if(ret = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM)){
  perror("Error making bound thread [pthread_attr_setscope(3C)] ");
  return (-1);
 }

 //初始化用于读线程池的线程，开启两个线程来完成任务，两个线程会互斥地访问任务链表
 for (i = 0; i < NETWORK_READ_THREAD_COUNT; i++){

  pthread_create(&tids[i], &tattr, thread_read_tasks, NULL);
  //log("new read task thread %8X created.", tids[i]);
 }

#if VERSION_SOLARIS
 epfd = port_create();
#else
 epfd = epoll_create(MAX_EPOLL_EVENT_COUNT);
#endif

 struct sockaddr_in serveraddr;

 if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){

  log("can't create socket.\n");

  return false;
 }

 //把socket设置为非阻塞方式
 _setnonblocking(listenfd);

 optionVal = 0;
 setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,  &optionVal, sizeof(optionVal));

 {
  //设置与要处理的事件相关的文件描述符
  context = query_context(listenfd, "localhost", SERV_PORT);

#if VERSION_SOLARIS
  //注册epoll事件
  port_associate(epfd, PORT_SOURCE_FD, listenfd, POLLIN, context);
#else
  ev.data.ptr = context;
  ev.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
#endif
 }

 memset(&serveraddr, 0, sizeof(serveraddr));
 serveraddr.sin_family = AF_INET;
 serveraddr.sin_port = htons(SERV_PORT);
 serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

 if (bind(listenfd,(sockaddr *)&serveraddr, sizeof(serveraddr))  == -1){

  log("bind error: %d\n", errno);

  return false;
 }

 //开始监听
 if (listen(listenfd, LISTENQ) == -1){

  log("listen error: %d\n", errno);

  return false;
 }

 log("");
 log("**************************************");
 log("********   cserver start ok   ********");
 log("**************************************");
 
 pthread_t tids_get_network_event[NETWORK_READ_THREAD_COUNT];

 for (int i = 1; i <= 2; i++){
  pthread_create(&tids_get_network_event[i], &tattr, get_network_event, (void*)i);
 }

 get_network_event(NULL);

 return true;
}

void shutdown_network()
{
 log("begin shutdown network ...");
}

void *thread_read_tasks(void *args)
{
 bool is_listenfd;
 st_context *context;

 while(1)
 {
  //互斥访问任务队列
  pthread_mutex_lock(&mutex_thread_read);

  //等待到任务队列不为空
  while(read_tasks_head == NULL)
   pthread_cond_wait(&cond_thread_read, &mutex_thread_read); //线程阻塞，释放互斥锁，当等待的条件等到满足时，它会再次获得互斥锁

  context     = read_tasks_head->context;

  //从任务队列取出一个读任务
  st_io_context_task *tmp = read_tasks_head;
  read_tasks_head = read_tasks_head->next;
  delete tmp;

  if (read_tasks_head == NULL){
   read_tasks_tail = NULL;
  }

  pthread_mutex_unlock(&mutex_thread_read);

  {
   char buf_read[MAX_READ_BUF_SIZE];
   int  read_count;

   read_count = recv(context->fd, buf_read, sizeof(buf_read), 0);

   //log("read id[%d]errno[%d]count[%d]", context->id, errno, read_count);

   if (read_count < 0)
   {
    if (errno == EAGAIN){
     continue;
    }

    log("1 recv < 0: errno: %d", errno);
    //if (errno == ECONNRESET){
    // log("client[%s:%d] disconnect ", context->remote_addr, context->remote_port);
    //}

    del_from_network(context);
    continue;
   }
   else if (read_count == 0)
   {
    //客户端关闭了，其对应的连接套接字可能也被标记为EPOLLIN，然后服务器去读这个套接字
    //结果发现读出来的内容为0，就知道客户端关闭了。
    log("client close connect! errno[%d]", errno);

    del_from_network(context);
    continue;
   } 
   else
   {
    do 
    {
     if (! on_recv_data(context, buf_read, read_count)){
      context->is_illegal = true;

      log("当前客户数据存在异常");
      del_from_network(context);
      break;
     }

     read_count = read(context->fd, buf_read, sizeof(buf_read));

     if (read_count <= 0){
      if (errno == EINTR)
       continue;
      if (errno == EAGAIN){
#if VERSION_SOLARIS
       port_associate(epfd, PORT_SOURCE_FD, context->fd, POLLIN, context);
#endif
       break;
      }


      log("2 error read_count < 0, errno[%d]", errno);

      del_from_network(context);
      break;
     }
    }
    while(1);
   }
  }
 }

 log("thread_read_tasks end ....");
}

void del_from_network(st_context *context, bool is_card_map_locked)
{
 gdata.lock();
 if (gdata.manager == context){
  gdata.manager = NULL;
 }
 gdata.unlock();

 context->lock();

 if (! context->valide){
  log("del_from_network is not valide");
  context->unlock();
  return;
 }

 // 断开连接
 int fd = context->fd;

 if (fd != -1){
#if VERSION_SOLARIS
  port_dissociate(epfd, PORT_SOURCE_FD, fd);
#else
  struct epoll_event ev;
  ev.data.fd = fd;
  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
#endif
  close_socket(fd);
  context->fd = -1;
 }

 context->valide = false;
 log("client[%d] is invalide", context->id);
 context->unlock();

 mutex_lock(mutex_id_pool.mutex);
 del_client(context->id);
 mutex_unlock(mutex_id_pool.mutex);
}

bool context_write(st_context *context, const char* buf, int len)
{
 if (len <= 0) return true;

 int fd = context->fd;

 if (fd == -1){
  return false;
 }

 int nleft = len;
 int nsend;

 bool result = true;

    while(nleft > 0){
        nsend = write(fd, &buf[len - nleft], nleft);

        if (nsend < 0) {

            if(errno == EINTR) continue;
   if(errno == EAGAIN) {
    break;
   }

            result = false;
   break;
        }
        else if (nsend == 0){

            result = false;
   break;
        }
        nleft -= nsend;
    }

 return result;
}

