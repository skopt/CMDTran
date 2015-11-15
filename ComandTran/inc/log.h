#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/*
must be called first
*/
#define LogInf(info) CLog::Instance.LogWrite(__FILE__, __LINE__, __FUNCTION__, info);

class CLog
{
public:
    CLog();
    ~CLog();
    int LogWrite(const char* file, int line, const char* fun, const char* info); 
private:
    void Init();
    int GetCurrentTime(char *ptime);
public:
    static CLog Instance;

private:
    pthread_mutex_t flock;
    int fd;
};

#endif
