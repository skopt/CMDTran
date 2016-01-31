#ifndef _FRAMERESTRUCT_H
#define _FRAMERESTRUCT_H

#include <list>  
#include "Task.h"

using namespace std; 

#define EXIT_BUFFER_LEN 2048
#define FRAME_HEAD 0x7E
#define FRAME_LEN_MIN 5
#define FRAME_LEN_MAX 1024 + 100
class CFrameRestruct{
public:
    CFrameRestruct();
    ~CFrameRestruct();
    int RestructFrame(int sock, char *pRecvBuffer, int RecvLen);
private:
    int GetFrame(int sock, char *pRecvBuffer, int RecvLen);
    char* GetBuffer();
    void FreeBuffer(char* buff);
    void AddToList(int sock, char *pFrame, int len);
    int GetHeadIndex(int CurIndex, char *pRecvBuffer, int RecvLen);
    int GetFrameLen(int CurIndex, char *pRecvBuffer, int RecvLen);
    void CopyRestToExitBuff(int CurIndex, char *pRecvBuffer, int RecvLen);

    bool CheckSum(int CurIndex, char *pRecfBuffer, int FrameLen);
    bool SaveFrame(int sock, int CurIndex, char *pRecvBuffer, int FrameLen);

public:
    void *pRecvDataProc;

private:
    char m_pExitBuffer[EXIT_BUFFER_LEN];
    int m_iExitLen;	
public:
    list< CTask* > TaskList;
};
#endif
