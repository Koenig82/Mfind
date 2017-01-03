#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

typedef struct threadArg{
    queue* directories;
    char* filter;
}threadArg;
void getDir(int argc, char **argv, int nrArg, threadArg* arg);
void search(threadArg* arg);