#include "BasicFun.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


string Int2String(int value)
{
    char buf[16];
    memset(buf, 0, 16);
    sprintf(buf, "%d", value);
    string ret = buf;
    return ret;
}

void NSleep(int interval)
{
    timespec spec;
    spec.tv_sec = 0;
    spec.tv_nsec = interval;
    nanosleep(&spec, NULL);
}

void USleep(int interval)
{
    NSleep(interval * 1000);
}

