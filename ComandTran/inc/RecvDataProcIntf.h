#ifndef _RECV_DATA_PROC_INTF_H_
#define _RECV_DATA_PROC_INTF_H_

//#include "EpollServer.h"
#include "MemChain.h"

class CEpollServer;
class CRecvDataProcIntf
{
public:
    CRecvDataProcIntf();
    virtual ~CRecvDataProcIntf();
protected:
     CEpollServer* m_pEpoll;
     bool SendData(int sock, char* buff, int len, SendCallBack backfun);
     bool EnableRead(int sock);
     bool DisableRead(int sock);
public:
    void SetEpoll(CEpollServer* epoll);
    virtual void AddClient(int sock)=0;
    virtual void QuitClient(int sock)=0;
    virtual bool AddRecvData(int sock, char *pbuff, int len)=0;
};

#endif