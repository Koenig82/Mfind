#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "queue.h"
#include <pthread.h>
#include <sys/stat.h>

typedef struct threadArg{
    queue* directories;
    char* filter;
    unsigned int* nrOfThreads;
    int* waitLock;
    pthread_mutex_t* mutex;
    pthread_barrier_t* barrier;
    pthread_cond_t* condition;
}threadArg;

typedef struct {
    unsigned int searched;
    threadArg* arg;
} threadContext;

void getDir(int argc, char **argv, int nrArg, threadArg* arg);
void* search(void* arg);