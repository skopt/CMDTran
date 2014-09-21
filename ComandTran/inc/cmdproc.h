#ifndef _CMDPROC_
#define _CMDPROC_
#include <unistd.h>

typedef int (*ComdRetPro)(char *str);
int CommandPro(char *Command, ComdRetPro ProFun);


class Cmdproc 
{
private:

public:
	static int RecvDataProc(char *Command, int len, int Confd);
};
#endif