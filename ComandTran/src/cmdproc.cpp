#include <stdio.h>
#include "cmdproc.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#define BUFFLEN 2048

int CommandPro(char *Command, ComdRetPro ProFun)
{
    FILE *fstr;
    char buff[BUFFLEN];
    memset(buff, 0, BUFFLEN);

    fstr = popen(Command, "r");

    if(NULL == fstr)
        return 0;

    while(NULL != fgets(buff, BUFFLEN, fstr))
    {
        ProFun(buff);
    }

    pclose(fstr);   

    return 1;
}

int Cmdproc::RecvDataProc(char *Command, int len, int Confd)
{
	FILE *fstr;
    char buff[BUFFLEN];
    memset(buff, 0, BUFFLEN);

	send(Confd, Command, strlen(Command), 0);
	send(Confd, "\n", strlen("\n"), 0);

    fstr = popen(Command, "r");    
    if(NULL == fstr)
        return 0;

    while(NULL != fgets(buff, BUFFLEN, fstr))
    {
        send(Confd, buff, strlen(buff), 0);
    }

    pclose(fstr);

	send(Confd, "\n\n", strlen("\n\n"), 0);

    return 0;
}
