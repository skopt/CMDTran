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

bool CRecvDataProcIntf::DisableRead(int sock)
{
    return m_pEpoll->DisableRead(sock);
}

bool CRecvDataProcIntf::EnableRead(int sock)
{
    return m_pEpoll->EnableRead(sock);
}

void CRecvDataProcIntf::SetEpoll(CEpollServer* epoll)
{
    m_pEpoll = epoll;
}