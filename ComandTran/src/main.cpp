#include <stdio.h>
#include <string.h>
#include <cmdproc.h>
#include "EpollServer.h"

int CommandResultPro(char *ret);

int main()
{
       char buff[1024];
	CEpollServer tmp(6000);
	tmp.Start();
	
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
