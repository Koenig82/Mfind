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
    bool* running;
    pthread_mutex_t* queueMut;
    pthread_mutex_t* condMut;
    pthread_barrier_t* barrier;
    pthread_cond_t* condition;
}threadArg;

typedef struct {
    unsigned int searched;
    long int id;
    threadArg* shared;
} threadContext;

void getDir(int argc, char **argv, int nrArg, threadArg* arg);
void* search(void* arg);