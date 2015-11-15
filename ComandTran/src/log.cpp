#include "log.h"
#include <time.h>
#include <string.h>
#include <pthread.h>

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

int CLog::LogWrite(const char* file, int line, const char* fun, const char* info)
{
    int ret =0;
    char buf[BUF_LEN];
    memset(buf, 0, BUF_LEN);

    time_t now;
    struct tm *time_now;
    now = time(&now);
    time_now = localtime(&now);

    int len = snprintf(buf, BUF_LEN, "[%02d-%02d-%02d %02d:%02d:%02d] (%s:%d)-%s--%s\n"
        , time_now->tm_year + 1990, time_now->tm_mon, time_now->tm_mday, time_now->tm_hour
        , time_now->tm_min, time_now->tm_sec
        , file, line, fun, info);
   
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
