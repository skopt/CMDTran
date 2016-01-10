#include "FrameRestruct.h"
#include "RecvDataProc.h"
#include <string.h>
#include "Log.h"

#define LogI printf

CFrameRestruct::CFrameRestruct()
:m_iExitLen(0)
{
    memset(m_pExitBuffer, 0, EXIT_BUFFER_LEN);
}

CFrameRestruct::~CFrameRestruct()
{
}

int CFrameRestruct::RestructFrame(int sock, char *pRecvBuffer, int RecvLen)
{
	int v_iFrameCount = 0;
	int v_iCurIndex = 0;
	int v_iFrameLen = 0;
	char *pFrame = NULL;
	while(true)
	{
		//check head
		if(v_iCurIndex > m_iExitLen + RecvLen)
		{
			m_iExitLen = 0;
			break;
		}
		//get head
		v_iCurIndex = GetHeadIndex(v_iCurIndex, pRecvBuffer, RecvLen);
		if(-1 == v_iCurIndex)//no frames in rest buffer
		{
			m_iExitLen = 0;
			v_iFrameCount++;
			break;
		}
		//check if the shortest frame
		if(v_iCurIndex + FRAME_LEN_MIN > m_iExitLen + RecvLen)
		{
			CopyRestToExitBuff(v_iCurIndex, pRecvBuffer, RecvLen);
			break;
		}
		//get frame len
		v_iFrameLen = GetFrameLen(v_iCurIndex, pRecvBuffer, RecvLen);
		if(v_iCurIndex + v_iFrameLen > m_iExitLen + RecvLen)
		{
			CopyRestToExitBuff(v_iCurIndex, pRecvBuffer, RecvLen);
			break;
		}
		if(-1 == v_iFrameLen)
		{
			v_iCurIndex++;
			continue;
		}
		//checksum
		if(!CheckSum(v_iCurIndex, pRecvBuffer,v_iFrameLen))
		{
			v_iCurIndex++;
			continue;
		}
		//save frame
		if(SaveFrame(sock,v_iCurIndex, pRecvBuffer,v_iFrameLen))
		    v_iFrameCount++;
        
		v_iCurIndex += v_iFrameLen;
	}

	return v_iFrameCount;
}

char* CFrameRestruct::GetBuffer()
{
	if(NULL == pRecvDataProc)
		return NULL;
	
	CRecvDataProc *v_pRecvDataProc = (CRecvDataProc *) pRecvDataProc;
	if(NULL == v_pRecvDataProc)
	{
		LogI("v_pRecvDataProc is null\n");
		return NULL;
	}
	return v_pRecvDataProc->GetBuff();
}
void CFrameRestruct::FreeBuffer(char* buff)
{
	if(NULL == pRecvDataProc)
		return ;
	
	CRecvDataProc *v_pRecvDataProc = (CRecvDataProc *) pRecvDataProc;
	if(NULL == v_pRecvDataProc)
	{
		LogI("v_pRecvDataProc is null\n");
		return ;
	}
       v_pRecvDataProc->FreeBuff(buff);
}
void CFrameRestruct::AddToList(int sock, char *pFrame, int len)
{
	if(NULL == pRecvDataProc)
		return;

	CRecvDataProc *v_pRecvDataProc = (CRecvDataProc *) pRecvDataProc;
       v_pRecvDataProc->AddFrameProcTask(sock, pFrame, len);
}

int CFrameRestruct::GetHeadIndex(int CurIndex, char *pRecvBuffer, int RecvLen)
{
	int v_iHeadIndex = -1;
	for(int i = CurIndex; i < m_iExitLen + RecvLen; i++)
	{
		if(i < m_iExitLen)//in the m_pExitBuffer
		{
			if(m_pExitBuffer[i] == FRAME_HEAD)
			{
				v_iHeadIndex = i;
				break;
			}
		}
		else//in the recv buffer
		{
			if(pRecvBuffer[i - m_iExitLen] == FRAME_HEAD)
			{
				v_iHeadIndex = i;
				break;
			}			
		}
	}//end of for

	return v_iHeadIndex;
}

void CFrameRestruct::CopyRestToExitBuff(int CurIndex, char *pRecvBuffer, int RecvLen)
{	
	if(CurIndex < m_iExitLen)
	{
		int index = 0;
		for(int i = CurIndex; i < m_iExitLen; i++)
		{
			m_pExitBuffer[index++] = m_pExitBuffer[i];
		}
		memcpy(m_pExitBuffer + index, pRecvBuffer, RecvLen);
		m_iExitLen = index + RecvLen;
	}
	else
	{
		memcpy(m_pExitBuffer, pRecvBuffer + (CurIndex - m_iExitLen), m_iExitLen + RecvLen - CurIndex);
		m_iExitLen = m_iExitLen + RecvLen - CurIndex;
	}
}

int CFrameRestruct::GetFrameLen(int CurIndex, char *pRecvBuffer, int RecvLen)
{
	int v_iFrameLen = -1;
    if(CurIndex < m_iExitLen - 2)//len bytes in the m_pExitBuff
    {
		v_iFrameLen = ((unsigned char) m_pExitBuffer[CurIndex + 1]) * 256 + (unsigned char) m_pExitBuffer[CurIndex + 2];
    }
	else if(CurIndex == m_iExitLen - 2)
	{
		v_iFrameLen = ((unsigned char) m_pExitBuffer[CurIndex + 1]) * 256 + (unsigned char) pRecvBuffer[0];
	}
	else if(CurIndex > m_iExitLen -2)
	{
		v_iFrameLen = ((unsigned char) pRecvBuffer[CurIndex + 1 - m_iExitLen]) * 256 + (unsigned char) pRecvBuffer[CurIndex + 2 - m_iExitLen];
	}

	if(v_iFrameLen < FRAME_LEN_MIN || v_iFrameLen > FRAME_LEN_MAX)
	{
		v_iFrameLen = -1;
	}
	return v_iFrameLen;
}
bool CFrameRestruct::CheckSum(int CurIndex, char *pRecfBuffer, int FrameLen)
{
	bool ret = false;
	char code = 0x00;
	if(CurIndex < m_iExitLen && CurIndex + FrameLen <= m_iExitLen)
	{
		code = m_pExitBuffer[CurIndex + FrameLen -1];
	}
	else
	{
		code = pRecfBuffer[CurIndex - m_iExitLen + FrameLen -1];
	}
	//check
	if(0xA5 == (unsigned char) code)
	{
		ret =true;
	}
	return ret;
}
bool CFrameRestruct::SaveFrame(int sock, int CurIndex, char *pRecvBuffer, int FrameLen)
{
	char *pFrame = GetBuffer();
	//char pFrame[2048];// = GetBuffer();
	if(NULL == pFrame)
	{
		LogError("do not get buffer");
		return false;
	}
	if(CurIndex < m_iExitLen && CurIndex + FrameLen <= m_iExitLen)
	{
		memcpy(pFrame, &m_pExitBuffer[CurIndex], FrameLen);
	}
	else if(CurIndex < m_iExitLen && CurIndex + FrameLen > m_iExitLen)
	{
		memcpy(pFrame, &m_pExitBuffer[CurIndex], m_iExitLen - CurIndex);
		memcpy(pFrame + m_iExitLen - CurIndex, pRecvBuffer, FrameLen - (m_iExitLen - CurIndex));
	}
	else
	{
		memcpy(pFrame, &pRecvBuffer[CurIndex - m_iExitLen], FrameLen);
	}
       //FreeBuffer(pFrame);
       AddToList(sock, pFrame, FrameLen);
       return true;
}