//Written by Niklas Königsson
//
//headerfile for mfind
//This includes the strucs needed to pass to the searchfunction
//
//cs: dv15nkn
//cas: nika0068
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "queue.h"
#include <pthread.h>
#include <sys/stat.h>

//struct containg pointers to information shared by the threads
typedef struct threadArg{
    queue* directories;
    char* target;
    unsigned int* nrOfThreads;
    int* waitLock;
    bool* running;
    pthread_mutex_t* queueMut;
    pthread_mutex_t* condMut;
    pthread_barrier_t* barrier;
    pthread_cond_t* condition;
}threadArg;
//struct containing the threadlocal information
typedef struct {
    unsigned int searched;
    long int id;
    threadArg* shared;
} threadContext;

void getDir(int argc, char **argv, int nrArg, threadArg* arg);
void* search(void* arg);