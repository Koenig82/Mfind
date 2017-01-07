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
    char* target;
    int* waitLock;
    unsigned int* nrOfThreads;
    bool* running;
    queue* directories;
    pthread_mutex_t* queueMut;
    pthread_mutex_t* condMut;
    pthread_barrier_t* barrier;
    pthread_cond_t* condition;
}threadArg;

//struct containing the threadlocal information
typedef struct {
    unsigned int searched;
    long int id;
    char type;
    threadArg* shared;
} threadContext;

//function to get initial directories and target from argument line
void getDir(int argc, char **argv, int nrArg, threadArg* arg);

//threadfunction to search for target files
void* search(void* arg);