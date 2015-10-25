#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/*
must be called first
*/
class CLog
{
public:
    CLog();
    ~CLog();
    int LogWrite(char *info, int len); 
private:
    void Init();
    int GetCurrentTime(char *ptime);
public:

private:
    pthread_mutex_t flock;
};

#endif
