#include "log.h"
#include <time.h>
#include <string.h>
#include <pthread.h>

int getCurrentTime(char *ptime);

void  CLog::Init()
{
    pthread_mutex_init(&flock, NULL);   
}

int CLog::LogWrite(char *info, int len)
{
    //init
    char *path="./aa.log";
    int fd = 0, ret = 0;
    ssize_t size = 0;
    char current_time[22];

    //lock
    pthread_mutex_lock(&flock);

    //open file
    fd = open(path, O_WRONLY | O_CREAT | O_APPEND);
    if(fd < 0)
    {
        return -1;
    }
    //write time
    current_time[0] = '[';
    GetCurrentTime(current_time + 1);    
    current_time[20] = ']';
    current_time[21] = ' ';
    ret = write(fd, current_time, 22);
    //write info
    ret = write(fd, (void *)info, (size_t)len);
    if(ret != (ssize_t)len)
    {
        close(fd);
        return -1;
    }

    //write complete
    close(fd);

    //unlock
    pthread_mutex_unlock(&flock);
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
