//Written by Niklas Königsson
//
//mfind
//Program to search one or more filetrees after a file with different amount
//of threads.
//Program arguments must be on the form:
//mfind [-t type] [-p nrthr] start1 [start2 ...] name

//type is the type of file(d = directory, f = file, l = link).
//nrthr is the total number of threads to be used(integer).
//start1-n is the folders to search.
//name is the target file.

//cs: dv15nkn
//cas: nika0068
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include "mfind.h"

int main(int argc, char *argv[]) {

    //variables
    int flag;
    int waiting = 0;
    int index;
    unsigned int nrthr = 1;
    char flagtype = 0;

    //parse flags in argument
    while ((flag = getopt(argc, argv, "t:p:")) != -1) {
        switch (flag) {
            case 't':
                flagtype = *optarg;
                if(flagtype != 'd' && flagtype != 'f' && flagtype != 'l'){
                    fprintf(stderr, "Invalid parameters!\n"
                            "Usage: mfind [-t type] [-p nrthr] start1"
                            " [start2 ...] name\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                nrthr = (unsigned int)atoi(optarg);
                if(nrthr == 0){
                    fprintf(stderr, "Invalid parameters!\n"
                            "Usage: mfind [-t type] [-p nrthr] start1"
                            " [start2 ...] name\n");
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Invalid parameters!\n"
                        "Usage: mfind [-t type] [-p nrthr] start1"
                        " [start2 ...] name\n");
                exit(EXIT_FAILURE);
        }
    }
    if (argc - 1 == optind || optind >= argc){
        fprintf(stderr, "Invalid parameters!\n"
                "Usage: mfind [-t type] [-p nrthr] start1"
                " [start2 ...] name\n");
        exit(EXIT_FAILURE);
    }
    //initialize shared threadarguments
    threadArg* arg = malloc(sizeof(threadArg));
    arg->directories = queue_empty();
    arg->running = (bool *) true;
    arg->waitLock = &waiting;

    pthread_mutex_t qMutex;
    pthread_mutex_t cMutex;
    pthread_cond_t cond;
    pthread_barrier_t threadBarrier;
    arg->queueMut = &qMutex;
    arg->condMut = &cMutex;
    arg->condition = &cond;
    arg->barrier = &threadBarrier;

    pthread_mutex_init(arg->queueMut ,NULL);
    pthread_mutex_init(arg->condMut ,NULL);
    pthread_cond_init(arg->condition, NULL);

    //get starting directories and searchtarget from argumentline
    getDir(argc, argv, optind, arg);
    //initialize threads
    arg->nrOfThreads = &nrthr;
    pthread_barrier_init(&threadBarrier, NULL, nrthr);
    pthread_t threads[nrthr];
    //initialize threadcontexts
    threadContext context[nrthr];
    context[0].shared = arg;
    context[0].searched = 0;
    context[0].type = flagtype;
    //start threads
    for(index = 1; (unsigned int)index < nrthr; index++){
        context[index].shared = arg;
        context[index].searched = 0;
        context[index].type = flagtype;
        if(pthread_create(&threads[index], NULL, search,
                          (void*)&context[index])){
            perror("pthread_create: \n");
            exit(EXIT_FAILURE);
        }
    }
    search(&context[0]);

    //join threads
    for(index = 1; (unsigned int)index < nrthr;index++){
        if(pthread_join(threads[index], NULL)){
            perror("pthread_join :");
        }
    }

    //print each thread's individual workload
    for(index = 0; (unsigned int)index < nrthr; index++){
        printf("\nThread: %ld Reads: %d",
               context[index].id ,context[index].searched);
    }
    //free memory
    pthread_mutex_destroy(&qMutex);
    pthread_barrier_destroy(&threadBarrier);
    pthread_cond_destroy(&cond);
    queue_free(arg->directories);
    free(arg);

}
/**
 * Main function for threads. searches folders from a queue
 * and matches it with a target string. The argument is a
 * threadContext struct that contains threadlocal information and a pointer
 * to a threadArg struct for global information.
 * @param args, threadcontext
 * @return void
 */
void* search(void* args){
    //variables
    DIR *dir = NULL;
    struct dirent *ent;
    struct stat st;
    //set threadcontext
    threadContext* context = (threadContext*)args;
    context->id = pthread_self();
    //gather upp all threads before launch
    pthread_barrier_wait(context->shared->barrier);
    while(context->shared->running){
        //if queue is empty...
        pthread_mutex_lock(context->shared->queueMut);
        if(queue_isEmpty(context->shared->directories)) {
            pthread_mutex_unlock(context->shared->queueMut);
            pthread_mutex_lock(context->shared->condMut);
            //...and you are the last thread:
            if((unsigned int)*context->shared->waitLock >=
                    (*context->shared->nrOfThreads - 1)){
                //...broadcast to all waiting threads and set running to false
                context->shared->running = false;
                pthread_cond_broadcast(context->shared->condition);
                pthread_mutex_unlock(context->shared->condMut);
                return NULL;
            //...and you are not the last thread:
            }else{
                //...go to wating status
                *context->shared->waitLock = (*context->shared->waitLock + 1);
                //**Waiting status loop**
                while (true){
                    //threads in waiting check the queue again if signaled
                    //unless the last thread has set the global running
                    //parameter off
                    pthread_cond_wait(context->shared->condition,
                                      context->shared->condMut);
                    *context->shared->waitLock =
                            (*context->shared->waitLock - 1);
                    //thread continue work
                    pthread_mutex_unlock(context->shared->condMut);
                    break;
                }
            }
        //if queue is not empty...
        } else{
            //...save information from queue locally and continue search
            char* path = malloc(strlen(
                    queue_front(context->shared->directories)) + 1);
            strcpy(path, queue_front(context->shared->directories));
            free(queue_front(context->shared->directories));
            queue_dequeue(context->shared->directories);
            pthread_mutex_unlock(context->shared->queueMut);
            context->searched++;

            //open directory and analyze
            dir = opendir(path);
            if (dir) {
                //total searchpath loop
                while ((ent = readdir(dir)) != NULL) {
                    char* fullpath = malloc(sizeof(char) *
                                            (strlen(path) +
                                             strlen(ent->d_name))+2);
                    strcpy(fullpath, path);
                    strcat(fullpath, ent->d_name);

                    if(lstat(fullpath, &st) != -1){
                    }else{
                        fprintf(stderr,"%s", path);
                        perror("lstat: ");
                        fflush(stderr);
                    }

                    //match with target and do work depending on filetype

                    //if readdir target is a symbolic link
                    if(S_ISLNK(st.st_mode)) {
                        if(context->type == 'l' || context->type == 0){
                            if(strcmp(ent->d_name, context->shared->target)
                               == 0){
                                char *symlink = malloc(1024);
                                memset(symlink, 0, 1024);
                                readlink(fullpath, symlink, 1023);
                                printf("\n%s", symlink);
                                free(symlink);
                            }
                        }
                    }
                    //if readdir target is a directory
                    else if(S_ISDIR(st.st_mode)){
                        //exclude . and .. folders
                        if(strcmp(ent->d_name, (char *) ".") != 0 &&
                           strcmp(ent->d_name, (char *) "..") != 0) {
                            strcat(fullpath, (char *) "/");
                            if(context->type == 'd' || context->type == 0){
                                if(strcmp(ent->d_name, context->shared->target)
                                   == 0){
                                    printf("\n%s%s", path, ent->d_name);
                                }
                            }
                            //enqueue the new folder
                            pthread_mutex_lock(context->shared->queueMut);
                            queue_enqueue(context->shared->directories,
                                          fullpath);
                            pthread_mutex_unlock(context->shared->queueMut);
                            //for each new folder found, signal a waiting thread
                            pthread_mutex_lock(context->shared->condMut);
                            pthread_cond_signal(context->shared->condition);
                            pthread_mutex_unlock(context->shared->condMut);
                            continue;
                        }
                    }
                    //if readdir target is a file
                    else if(S_ISREG(st.st_mode)){
                        if(context->type == 'f' || context->type == 0){
                            if(strcmp(ent->d_name, context->shared->target)
                               == 0){
                                printf("\n%s%s", path, ent->d_name);
                            }
                        }

                    }
                    free(fullpath);
                }
                closedir (dir);
            } else {
                /* could not open directory */
                fprintf(stderr, "could not open \"%s\"\n", path);
                perror("opendir:");
                fflush(stderr);
            }
            free(path);
        }
    }
    return NULL;
}
/**
 * This function fills the threadArg struct with information. Enqueues the
 * folders and sets the target from the mainarguments line.
 * @param argc main nr of arguments
 * @param argv main argumentarray
 * @param nrArg main number of arguments excluding flags
 * @param arg pointer to threadArg struct to fill
 */
void getDir(int argc, char **argv, int nrArg, threadArg* arg){

    for(; nrArg < (argc-1); nrArg++){
        char* path = malloc(strlen(argv[nrArg])+2);
        strcpy(path, argv[nrArg]);
        if(path[strlen(path)] != '/'){
            strcat(path, "/");
        }
        queue_enqueue(arg->directories, path);
    }
    arg->target = argv[nrArg];
}
