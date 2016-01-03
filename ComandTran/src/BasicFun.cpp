#include "BasicFun.h"
#include <stdio.h>
#include <string.h>


string Int2String(int value)
{
    char buf[16];
    memset(buf, 0, 16);
    sprintf(buf, "%d", value);
    string ret = buf;
    return ret;
}

