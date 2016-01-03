#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include <pthread.h>
using namespace std;  
#include <list>  
#include <map>
#include <stdio.h>
#include <string>

#include "Task.h"
#include "LockFreeQueue.h"
//---------------define---------------------------
typedef void (*TaskProcess)(void *pArgu);
typedef bool (*GetTaskCustomized)(void *pArgu);
#define LogE printf
#define LogD printf
#define LogI printf

class CThreadPool
{
public:
	CThreadPool(string name);
	~CThreadPool();
private:
	int RuningFlag;
	CLockFreeQueue< CTask* > TaskList;
	pthread_t *ThreadCreated;        //all the thread created
	int mThreadCount;                    //thread count
	string m_Name;
public:
      	GetTaskCustomized  GetTask_Cust;
private:
	static void* _ThreadRoutine(void *pArgu);
public:
	bool InitPool(int threadCount);
	bool AddTask(CTask* addTask);
	CTask* GetTask();
	bool ShutDown();
};

#endif
