#ifndef _TASK_H
#define _TASK_H
class CTask
{
public:
	CTask(){}
        ~CTask(){}
public:
	virtual void ProcessTask() = 0;
};

#endif

