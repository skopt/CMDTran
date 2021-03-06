#include "Log.h"
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>


#define BUF_LEN 2048
CLog CLog::Instance;

int getCurrentTime(char *ptime);

CLog::CLog()
{
    char *path="./cmd.log";
    fd = open(path, O_RDWR | O_CREAT | O_APPEND, S_IRWXU);
    pthread_mutex_init(&flock, NULL);  
    
}
CLog::~ CLog()
{
    close(fd);
}
void  CLog::Init()
{
 
}

int CLog::LogWrite(int tid, const char* file, int line, const char* fun, const char* fmt,...)
{
    int ret =0;
    char buf[BUF_LEN];
    char info[BUF_LEN - 100];
    memset(buf, 0, BUF_LEN);
    memset(info, 0, BUF_LEN - 100);

    va_list argu;
    va_start(argu, fmt);
    vsnprintf(info, BUF_LEN - 100, fmt, argu);
    va_end(argu);

    time_t now;
    struct tm *time_now;
    now = time(&now);
    time_now = localtime(&now);

    int len = snprintf(buf, BUF_LEN, "[%02d-%02d-%02d %02d:%02d:%02d] (%s:%d)-%d-%s--%s\n"
        , time_now->tm_year + 1990, time_now->tm_mon, time_now->tm_mday, time_now->tm_hour
        , time_now->tm_min, time_now->tm_sec
        , file, line, tid, fun, info);
   
    ret = write(fd, (void *)buf, (size_t)len);

    return 0;
}
/*
  return
  ptime format YYYY:MM:DD HH:mm:ss  19character
  0:success -1 error
*/
int CLog::GetCurrentTime(char *ptime)
{
    time_t now;
    struct tm *time_now;

    //get system time
    now = time(&now);
    //chang to local time
    time_now = localtime(&now);

    sprintf(ptime, "%02d-%02d-%02d %02d:%02d:%02d", time_now->tm_year + 1990, time_now->tm_mon,
        time_now->tm_mday, time_now->tm_hour, time_now->tm_min, time_now->tm_sec);
}
