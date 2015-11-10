#include "RecvDataProcIntf.h"
#include "EpollServer.h"
CRecvDataProcIntf::CRecvDataProcIntf()
{
}
CRecvDataProcIntf::~CRecvDataProcIntf()
{
}
bool CRecvDataProcIntf::SendData(int sock, char* buff, int len, SendCallBack backfun)
{
    
   return m_pEpoll->SendData(sock, buff, len, backfun);
};
void CRecvDataProcIntf::SetEpoll(CEpollServer* epoll)
{
    m_pEpoll = epoll;
}