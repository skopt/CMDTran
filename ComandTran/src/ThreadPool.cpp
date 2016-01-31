#include "ThreadPool.h"
#include "Log.h"
#include <sys/prctl.h>

CThreadPool::CThreadPool(string name)
:m_Name(name)
{
	RuningFlag = true;
	GetTask_Cust = NULL;
	TaskList.clear();
}

CThreadPool::~ CThreadPool()
{
}

bool CThreadPool::InitPool(int threadCount)
{
    pthread_mutex_init(&TaskListLock, NULL);
    pthread_cond_init(&TaskListReady, NULL);

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
    pthread_mutex_lock(&TaskListLock);
    TaskList.push_back(addTask);
    pthread_mutex_unlock(&TaskListLock);
    pthread_cond_signal(&TaskListReady);
    
    //addTask->ProcessTask();
    return true;
}
bool CThreadPool::AddTaskBatch(list< CTask* > & tasks)
{
    pthread_mutex_lock(&TaskListLock);
    list< CTask* >::iterator it;
    for(it = tasks.begin(); it != tasks.end(); ++it)
    {
        TaskList.push_back(*it);
        //CTask* tmp = *it;
        //tmp->ProcessTask();
    }
    pthread_mutex_unlock(&TaskListLock);
    pthread_cond_signal(&TaskListReady);
}

CTask* CThreadPool::GetTask()
{
    CTask* ret;
    pthread_mutex_lock(&TaskListLock);
    while(TaskList.empty() && RuningFlag)
    {
    	//LogI("Thread %d is waiting\n", pthread_self());
    	pthread_cond_wait(&TaskListReady, &TaskListLock);
    	//LogI("Thread %d weakup\n", pthread_self());
    }

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
    	typename list< CTask* >::iterator it;
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

bool CThreadPool::ShutDown()
{
    //set flag to be false
    while(RuningFlag)
    {
    	RuningFlag = false;
    }
    //send broadcast
    pthread_cond_broadcast(&TaskListReady);
    LogInf("send exit broadcast");
    for(int i = 0; i < mThreadCount; i++)
    {
    	pthread_join(ThreadCreated[i], NULL);
    }
    LogInf("all the thread exit");
    return true;
}
