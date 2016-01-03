#include "ThreadPool.h"
#include "Log.h"
#include <sys/prctl.h>

CThreadPool::CThreadPool(string name)
:m_Name(name)
{
	RuningFlag = true;
	GetTask_Cust = NULL;
}

CThreadPool::~ CThreadPool()
{
}

bool CThreadPool::InitPool(int threadCount)
{
    mThreadCount = threadCount;
    ThreadCreated = new pthread_t[mThreadCount];
    for(int i = 0; i < mThreadCount; i++)
    {
    	if(0 != pthread_create(&ThreadCreated[i], NULL, _ThreadRoutine, (void *)this))
    	{
    		LogError("creat thread failed, i=%d",i);
    		RuningFlag = false;//stop the thread running
    		return false;
    	}		
    }
    LogInf("creat %d threads",mThreadCount);

    return true;
}

void* CThreadPool::_ThreadRoutine(void *pArgu)
{
    CThreadPool *pthis = (CThreadPool *)pArgu;
    CTask* workTask;
    prctl(PR_SET_NAME, pthis->m_Name.c_str(), NULL, NULL, NULL);
    //get task
    while(true)
    {
    	workTask = pthis->GetTask();
    	if(false == pthis->RuningFlag)
    	{
    		LogDebug("Thread %d, RuningFlag is false, exit\n", pthread_self());
    		break;
    	}
    	if(NULL == workTask)
    	{
    		LogDebug("Process fun is null,continue\n");
    		continue;
    	}
    	workTask->ProcessTask();
    }
    pthread_exit (NULL);//for exit complete
}

bool CThreadPool::AddTask(CTask* addTask)
{
    return TaskList.Push(addTask);
}

CTask* CThreadPool::GetTask()
{
    CTask* ret;
    while(TaskList.IsEmpty() && RuningFlag)
    {
    	//LogI("Thread %d is waiting\n", pthread_self());
    	TaskList.Sleep();
    	//LogI("Thread %d weakup\n", pthread_self());
    }
    
    if(!RuningFlag)//exit
    {
    	return ret;
    }
    
    if(!TaskList.Pop(ret))
         ret = NULL;
    
    return ret;
}

bool CThreadPool::ShutDown()
{
    //set flag to be false
    while(RuningFlag)
    {
    	RuningFlag = false;
    }
    //send broadcast
    LogInf("send exit broadcast");
    for(int i = 0; i < mThreadCount; i++)
    {
    	pthread_join(ThreadCreated[i], NULL);
    }
    LogInf("all the thread exit");
    return true;
}
