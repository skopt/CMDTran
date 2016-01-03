#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)

/*
must be called first
*/
#define LogInf(...) CLog::Instance.LogWrite(gettid(), __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define LogDebug(...) CLog::Instance.LogWrite(gettid(), __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define LogError(...) CLog::Instance.LogWrite(gettid(),  __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

class CLog
{
public:
    CLog();
    ~CLog();
    int LogWrite(int tid, const char* file, int line, const char* fun, const char* fmt, ...); 
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
