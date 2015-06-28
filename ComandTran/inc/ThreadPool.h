#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include <pthread.h>
using namespace std;  
#include<list>  
#include <stdio.h>
//---------------define---------------------------
typedef void (*TaskProcess)(void *pArgu);
typedef bool (*GetTaskCustomized)(void *pArgu);
#define LogE printf
#define LogD printf
#define LogI printf

//----------------struct--------------------------
template<typename TaskVal>
class Task
{
public:
	Task(){}
	Task(TaskVal value, TaskProcess fun)
		:val(value),ProcessFun(fun)
		{}
	Task(const Task &tmp)
	{
		val = tmp.val;
		ProcessFun = tmp.ProcessFun;
	}
public:
	TaskVal val;
	TaskProcess ProcessFun;
};

template<typename TaskVal>
class CThreadPool
{
public:
	CThreadPool();
	~CThreadPool();
private:
	pthread_mutex_t TaskListLock;  
       pthread_cond_t TaskListReady; 
	int RuningFlag;
	list< Task<TaskVal> > TaskList;
	pthread_t *ThreadCreated;        //all the thread created
	int mThreadCount;                    //thread count
public:
      	GetTaskCustomized  GetTask_Cust;
private:
	static void* _ThreadRoutine(void *pArgu);
public:
	bool InitPool(int threadCount);
	bool AddTask(Task<TaskVal> addTask);
	Task<TaskVal> GetTask();
	bool ShutDown();
};

template<typename TaskVal>
CThreadPool<TaskVal>::CThreadPool()
{
	RuningFlag = true;
	GetTask_Cust = NULL;
	TaskList.clear();
}

template<typename TaskVal>
CThreadPool<TaskVal>::~ CThreadPool()
{
}

template<typename TaskVal>
bool CThreadPool<TaskVal>::InitPool(int threadCount)
{
	pthread_mutex_init(&TaskListLock, NULL);
	pthread_cond_init(&TaskListReady, NULL);

	mThreadCount = threadCount;
	ThreadCreated = new pthread_t[mThreadCount];
	for(int i = 0; i < mThreadCount; i++)
	{
		if(0 != pthread_create(&ThreadCreated[i], NULL, _ThreadRoutine, (void *)this))
		{
			LogE("creat thread failed, i=%d\n",i);
			RuningFlag = false;//stop the thread running
			return false;
		}		
	}
	LogI("creat %d threads\n",mThreadCount);

	return true;
}

template<typename TaskVal>
void* CThreadPool<TaskVal>::_ThreadRoutine(void *pArgu)
{
	CThreadPool *pthis = (CThreadPool *)pArgu;
	Task<TaskVal> workTask;
	//get task
	while(true)
	{
		workTask = pthis->GetTask();
		if(false == pthis->RuningFlag)
		{
			LogD("Thread %d, RuningFlag is false, exit\n", pthread_self());
			return NULL;
		}
		if(NULL == workTask.ProcessFun)
		{
			LogE("Process fun is null,continue\n");
			continue;
		}

		workTask.ProcessFun((void *)&workTask.val);
	}
    pthread_exit (NULL);//for exit complete
}

template<typename TaskVal>
bool CThreadPool<TaskVal>::AddTask(Task<TaskVal> addTask)
{
	pthread_mutex_lock(&TaskListLock);
	TaskList.push_back(addTask);
	pthread_mutex_unlock(&TaskListLock);
	pthread_cond_signal(&TaskListReady);
	return true;
}

template<typename TaskVal>
Task<TaskVal> CThreadPool<TaskVal>::GetTask()
{
	Task<TaskVal> ret;
	pthread_mutex_lock(&TaskListLock);
	while(TaskList.empty() && RuningFlag)
	{
		//LogI("Thread %d is waiting\n", pthread_self());
		pthread_cond_wait(&TaskListReady, &TaskListLock);
		//LogI("Thread %d weakup\n", pthread_self());
	}
       //LogD("Thread %d to work\n", pthread_self());

	if(!RuningFlag)//exit
	{
		pthread_mutex_unlock(&TaskListLock);
		return ret;
	}
       if(NULL == GetTask_Cust)
       {
		ret = TaskList.front();
		TaskList.pop_front();
       }
	else
	{
		typename list< Task<TaskVal> >::iterator it;
		for(it = TaskList.begin(); it != TaskList.end(); it++)
		{
			if(GetTask_Cust((void *)(&(*it))))
			{
				ret = *it;
				TaskList.erase(it);
				break;
			}
		}
	}
	pthread_mutex_unlock(&TaskListLock);
	return ret;
}

template <typename TaskVal>
bool CThreadPool<TaskVal>::ShutDown()
{
	//set flag to be false
	while(RuningFlag)
	{
		RuningFlag = false;
	}
	//send broadcast
	pthread_cond_broadcast(&TaskListReady);
	LogI("send exit broadcast\n");
	for(int i = 0; i < mThreadCount; i++)
	{
		pthread_join(ThreadCreated[i], NULL);
	}
	LogI("all the thread exit\n");
	return true;
}

#endif