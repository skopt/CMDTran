#include <stdio.h>
#include <string.h>
#include <cmdproc.h>
#include "EpollServer.h"
#include "RecvDataProc.h"

int CommandResultPro(char *ret);

int main()
{
    CRecvDataProc data_proc;
    CEpollServer server(6000, &data_proc);
    server.Start();

    //test
    char buff[1024];
    while(1)
    {
        scanf("%s", buff);
        CommandPro(buff,CommandResultPro); 
    }
    char *pComand = "ls";
    CommandPro(pComand,CommandResultPro);

    return 1;
}

int CommandResultPro(char *ret)
{
    printf("%s\n", ret);
}
