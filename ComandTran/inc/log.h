#ifndef _LOG_H
#define _LOG_H
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/*
must be called first
*/
int log_init();

int dbg_log(char *info);

/*
  return
  0:success
  other:error happen
*/
int log_write(char *info, int len); 

#endif
