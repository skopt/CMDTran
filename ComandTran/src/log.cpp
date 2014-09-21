#include "log.h"
#include <time.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t flock;
int getCurrentTime(char *ptime);

int log_init()
{
    if(pthread_mutex_init(&flock, NULL) != 0)
       return 1;
    else
        return 0;    
}
int dbg_log(char *info)
{
	if(NULL == info)
		return 0;
	log_write(info,strlen(info));
	return 1;
}
int log_write(char *info, int len)
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
    getCurrentTime(current_time + 1);    
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
int getCurrentTime(char *ptime)
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





