#ifndef _MASTER_WORKER_POOL_H
#define _MASTER_WORKER_POOL_H

#include <pthread.h>
using namespace std;  
#include <list>  
#include <map>
#include <stdio.h>
#include "Task.h"
#include "ThreadPool.h"

class CMasterWorkPool
{
public:
    CMasterWorkPool(string name);
    ~CMasterWorkPool();
    bool InitPool(int workercount);
    bool AddTask(int id, CTask* addTask);
    bool AddTaskBatch(int id, list<CTask*> & tasks);
    bool ShutDown();
private:
    
public:

private:
    int m_iWorkerCount;
    typedef map<int, CThreadPool*> ThreadMap;
    //typedef ThreadMap::value_type ThreadMapValue;
    ThreadMap m_ThreadMap;
    pthread_mutex_t m_ThreadMapLock;
    string m_Name;
};

#endif
