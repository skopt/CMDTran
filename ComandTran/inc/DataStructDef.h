#ifndef _DATASTRUCTDEF_H_
#define _DATASTRUCTDEF_H_
/*------------------Define-------------------------*/
struct MemBlock
{
	MemBlock *pNext;
	char *pBlock;
};

//the task value of recve data process
struct RecvFrameProcTV
{
	int RecvSocket;
	char *pFrame;
	int FameLen;
	void *pthis;
};

#endif
