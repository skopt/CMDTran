#include "MasterWorkPool.h"
#include "BasicFun.h"


CMasterWorkPool::CMasterWorkPool(string name)
:m_iWorkerCount(0), m_Name(name)
{
}

CMasterWorkPool::~CMasterWorkPool()
{
}


bool CMasterWorkPool::InitPool(int workercount)
{
    m_iWorkerCount = workercount;
    m_ThreadMap.clear();
    for(int i = 0; i < workercount; i++)
    {
        CThreadPool* pworker = new CThreadPool(m_Name + "_" + Int2String(i));
        m_ThreadMap.insert(pair<int, CThreadPool*>(i, pworker));
    }

    for(int i = 0; i < workercount; i++)
    {
        m_ThreadMap.find(i)->second->InitPool(1);//a worker one thread
    }
}

bool CMasterWorkPool::AddTask(int id, CTask* addTask)
{
    bool ret = true;
    int v_iWorkerId = id % m_iWorkerCount;
    m_ThreadMap.find(v_iWorkerId)->second->AddTask(addTask);
    return ret;
}
bool CMasterWorkPool::AddTaskBatch(int id, list<CTask*> & tasks)
{
    bool ret = true;
    int v_iWorkerId = id % m_iWorkerCount;
    m_ThreadMap.find(v_iWorkerId)->second->AddTaskBatch(tasks);
    return ret;
}

bool CMasterWorkPool::ShutDown()
{
    bool ret = true;
    for(int i = 0; i < m_iWorkerCount; i++)
    {
        m_ThreadMap.find(i)->second->ShutDown();
    }
    return ret;
}
