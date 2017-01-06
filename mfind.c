#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <getopt.h>
#include <string.h>
#include "mfind.h"

int main(int argc, char *argv[]) {

    //variables
    int flag;
    unsigned int nrthr = 0;
    int waiting = 0;
    int matchSet = false;
    int index;
    threadArg* arg = malloc(sizeof(threadArg));
    arg->directories = queue_empty();

    pthread_mutex_t qMutex;
    pthread_mutex_t cMutex;
    arg->queueMut = &qMutex;
    arg->condMut = &cMutex;
    pthread_mutex_init(arg->queueMut ,NULL);

    pthread_cond_t cond;
    arg->condition = &cond;
    pthread_cond_init(arg->condition, NULL);

    arg->running = true;

    char flagtype;


    //parse flags in argument
    //getopt verkar leta åt flaggor i argumentraden och lägger de i nån slags struktur
    //getint skapas...de är en int som pekar på n'ästa arg efter flaggorna
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
                //atoi gör om t.ex 4 och 0 till 40 om man skrivit att man vill ha 40 trådar
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
    //här är optind igen...lite skit för att få användaren att stoppa in rätt mängd argument
    //mycke är lånat från lorre
    if (argc - 1 == optind || optind >= argc){
        fprintf(stderr, "Invalid parameters!\n"
                "Usage: mfind [-t type] [-p nrthr] start1"
                " [start2 ...] name\n");
        exit(EXIT_FAILURE);
    }
    //funktionerna...under dom finns lite utskrifter om man vill undersöka flaggornas värde
    //men dom verka funka...tror man kan komma åt dom via optarg
    getDir(argc, argv, optind, arg);
    arg->nrOfThreads = &nrthr;
    arg->waitLock = &waiting;

    threadContext context[nrthr];

    pthread_barrier_t threadBarrier;
    arg->barrier = &threadBarrier;
    pthread_barrier_init(&threadBarrier, NULL, nrthr);
    pthread_t threads[nrthr];

    context[0].shared = arg;
    context[0].searched = 0;

    for(index = 1; index < nrthr; index++){
        context[index].shared = arg;
        context[index].searched = 0;
        if(pthread_create(&threads[index], NULL, search, (void*)&context[index])){
            perror("pthread_create: \n");
            exit(EXIT_FAILURE);
        }
    }

    search(&context[0]);
    for(index = 1; index < nrthr;index++){
        if(pthread_join(threads[index], NULL)){
            perror("pthread_join :");
        }
    }
    printf("\ntypeFlag: %c\nnrOfThreadFlag: %d ", flagtype, nrthr);

    printf("\nsearchFor: %s", arg->filter);
    for(index = 0; index < nrthr; index++){
        printf("\nthread %ld rearched: %d folders",context[index].id ,context[index].searched);
    }
    pthread_mutex_destroy(&qMutex);
    pthread_barrier_destroy(&threadBarrier);
    pthread_cond_destroy(&cond);
    queue_free(arg->directories);
    free(arg);


//    p_thread
//    2 = search(argv[1]);
//
//    joina trådar
//
//    skriv resultat

}
void* search(void* args){
    //struktur där man kan spara ner folders
    DIR *dir = NULL;
    //2 till strukturer där man kan lagra ner filer för att få info om dom
    struct dirent *ent;
    struct stat st;
    char* path = 0;
    char* filename;
    threadContext* context = (threadContext*)args;
    context->id = pthread_self();
    pthread_barrier_wait(context->shared->barrier);

    while(context->shared->running){
        pthread_mutex_lock(context->shared->queueMut);
        /*while(queue_isEmpty(context->shared->directories)){
            pthread_mutex_unlock(context->shared->queueMut);
            if(*context->shared->waitLock == (*context->shared->nrOfThreads - 1)){
                context->shared->running = false;
                pthread_cond_broadcast(context->shared->condition);
                //pthread_mutex_unlock(context->shared->queueMut);
                break;
            }else{
                *context->shared->waitLock = (*context->shared->waitLock + 1);
                pthread_cond_wait(context->shared->condition, context->shared->condMut);
                *context->shared->waitLock = (*context->shared->waitLock - 1);
                pthread_mutex_lock(context->shared->queueMut);
                continue;
            }
        }*/
        //pthread_mutex_lock(context->shared->mutex);
        if(!queue_isEmpty(context->shared->directories)) {
            printf("\n*** Behandlar katalog: %s ***\n", (char *) queue_front(context->shared->directories));
            path = malloc(strlen(queue_front(context->shared->directories)) + 1);
            strcpy(path, queue_front(context->shared->directories));
            free(queue_front(context->shared->directories));
            queue_dequeue(context->shared->directories);
            pthread_mutex_unlock(context->shared->queueMut);
            //pthread_cond_broadcast(context->shared->condition);
            dir = opendir(path);
            context->searched++;
        }else if(context->shared->running){
            pthread_mutex_unlock(context->shared->queueMut);
            //pthread_cond_broadcast(context->shared->condition);
            break;
        }
        //pthread_mutex_unlock(context->shared->queueMut);
        //pthread_cond_broadcast(context->shared->condition);
        //opendir öppnar en folder från sträng
        //dir = opendir(path);
        if (dir != NULL) {
            //readdir dundrar igenom nästa fil i en folder och sparar i en struct dirent
            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != NULL) {
                filename = malloc(strlen(path) + strlen(ent->d_name) +2);
                strcpy(filename, path);
                //addera filnamnet på katalogpathen
                strcat(filename, ent->d_name);

                //lstat sparar ner info i en liknande struktur som dirent...fast den har lite annan info
                //kanske nån är överflödig men ja tror fan de krävs båda för
                //att få ut allt man vill ha
                if(lstat(filename, &st) != -1){
                }else{
                    fprintf(stderr,"%s", path);
                    perror("lstat: ");
                    fflush(stderr);
                }
                printf ("\nfilename: %s\n", ent->d_name);
                //S_ISLINK och dom andra kollar om filen är link, fil eller dir
                if(S_ISLNK(st.st_mode)) {
                    printf("Type: Symbolic link\n");  //todo skriva ut resultat korrekt
                    printf("Path: %s\n", filename);
                }
                else if(S_ISDIR(st.st_mode)){
                    printf ("Type: Directory\n");     //todo skriva ut resultat korrekt
                    printf("path: %s\n", filename);
                    //om de är en dir och inte är . eller .. så ska den addera till kön o bränna av en ny opendir
                    if(strcmp(ent->d_name, (char *) ".") != 0 &&
                            strcmp(ent->d_name, (char *) "..") != 0) {
                        strcat(filename, (char *) "/");
                        printf("***köar på: %s\n", filename);
                        pthread_mutex_lock(context->shared->queueMut);
                        queue_enqueue(context->shared->directories, filename);
                        pthread_mutex_unlock(context->shared->queueMut);
                        //pthread_cond_broadcast(context->shared->condition);
                        continue;
                    }
                }
                else if(S_ISREG(st.st_mode)){
                    printf ("Type: File\n");        //todo skriva ut resultat korrekt
                    printf("path: %s\n", filename);
                }
                free(filename);
            }
            closedir (dir);
            dir = NULL;
        } else {
            /* could not open directory */
            fprintf(stderr, "%s", path);
            perror("opendir:");
            fflush(stderr);
        }
        free(path);
    }
    return NULL;
}

void getDir(int argc, char **argv, int nrArg, threadArg* arg){

    for(; nrArg < (argc-1); nrArg++){
        printf(">>>>Köar på: %s\n", argv[nrArg]);
        char* path = malloc(strlen(argv[nrArg])+1);
        strcpy(path, argv[nrArg]);
        queue_enqueue(arg->directories, path);
    }
    printf("\n ----------------  search for: %s ----------------", argv[nrArg]);
    arg->filter = argv[nrArg];
}