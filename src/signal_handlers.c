#include "signal_handlers.h"
#include <stdio.h>
#include <signal.h>

extern int signal_count;
void catch_sigint(int signalNo)
{
  printf(" : Input 'exit' to terminate this program\n");
  signal(SIGINT, (void*)catch_sigint);
  signal_count ++;
}

void catch_sigtstp(int signalNo)
{
  printf(" : Cannot Stop!\n");
 signal(SIGTSTP,(void*)catch_sigtstp);
 signal_count ++;
}

