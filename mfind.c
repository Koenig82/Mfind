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
    //int matchSet = false;
    int index;
    threadArg* arg = malloc(sizeof(threadArg));
    arg->directories = queue_empty();

    pthread_mutex_t qMutex;
    pthread_mutex_t cMutex;
    arg->queueMut = &qMutex;
    arg->condMut = &cMutex;
    pthread_mutex_init(arg->queueMut ,NULL);
    pthread_mutex_init(arg->condMut ,NULL);

    pthread_cond_t cond;
    arg->condition = &cond;
    pthread_cond_init(arg->condition, NULL);

    arg->running = (bool *) true;

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

    for(index = 1; (unsigned int)index < nrthr; index++){
        context[index].shared = arg;
        context[index].searched = 0;
        if(pthread_create(&threads[index], NULL, search, (void*)&context[index])){
            perror("pthread_create: \n");
            exit(EXIT_FAILURE);
        }
    }

    search(&context[0]);
    for(index = 1; (unsigned int)index < nrthr;index++){
        if(pthread_join(threads[index], NULL)){
            perror("pthread_join :");
        }
    }
    printf("\ntypeFlag: %c\nnrOfThreadFlag: %d ", flagtype, nrthr);

    printf("\nsearchFor: %s", arg->filter);
    for(index = 0; (unsigned int)index < nrthr; index++){
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
    threadContext* context = (threadContext*)args;
    context->id = pthread_self();
    pthread_barrier_wait(context->shared->barrier);

    while(context->shared->running){

        printf("0x%ld %s(%d)acquire queueMut\n", context->id, __FUNCTION__, __LINE__);
        pthread_mutex_lock(context->shared->queueMut);
        if(queue_isEmpty(context->shared->directories)) {
            printf("0x%ld %s(%d)release queueMut\n", context->id, __FUNCTION__, __LINE__);
            pthread_mutex_unlock(context->shared->queueMut);

            printf("\n*** (0x%ld) waitLock is:%d nrthr is:%d\n", context->id, *context->shared->waitLock, (*context->shared->nrOfThreads - 1));
            printf("0x%ld %s(%d)acquire condMut\n", context->id, __FUNCTION__, __LINE__);
            pthread_mutex_lock(context->shared->condMut);
            if((unsigned int)*context->shared->waitLock >= (*context->shared->nrOfThreads - 1)){
                context->shared->running = false;
                printf("\n*** %ld broadcast and exit FINAL\n", context->id);
                pthread_cond_broadcast(context->shared->condition);
                printf("0x%ld %s(%d)release condMut\n", context->id, __FUNCTION__, __LINE__);
                pthread_mutex_unlock(context->shared->condMut);
                return NULL;
            }else{
                printf("\n*** %ld wait\n", context->id);
                *context->shared->waitLock = (*context->shared->waitLock + 1);
                bool c = true;
                while (c){
                    printf("0x%ld %s(%d)acquire queueMut\n", context->id, __FUNCTION__, __LINE__);
                    pthread_mutex_lock(context->shared->queueMut);
                    c = queue_isEmpty(context->shared->directories);
                    printf("0x%ld %s(%d)release queueMut\n", context->id, __FUNCTION__, __LINE__);
                    pthread_mutex_unlock(context->shared->queueMut);
                    pthread_cond_wait(context->shared->condition, context->shared->condMut);
                    *context->shared->waitLock = (*context->shared->waitLock - 1);
                    if (!context->shared->running) {
                        printf("\n*** %ld exit after wait FINAL\n", context->id);
                        printf("0x%ld %s(%d)release condMut\n", context->id, __FUNCTION__, __LINE__);
                        pthread_mutex_unlock(context->shared->condMut);
                        break;
                    }
                    printf("\n*** %ld exit after wait\n", context->id);
                    printf("0x%ld %s(%d)release condMut\n", context->id, __FUNCTION__, __LINE__);
                    pthread_mutex_unlock(context->shared->condMut);
                    break;
                }
            }
        } else{
            printf("\n*** (0x%ld) Behandlar katalog: %s ***\n", context->id, (char *) queue_front(context->shared->directories));
            char* path = malloc(strlen(queue_front(context->shared->directories)) + 1);
            strcpy(path, queue_front(context->shared->directories));
            //printf("\n*** (0x%ld) Kopierade path: %s ***\n", context->id, path);

            free(queue_front(context->shared->directories));
            queue_dequeue(context->shared->directories);
            printf("0x%ld %s(%d)release queueMut\n", context->id, __FUNCTION__, __LINE__);
            pthread_mutex_unlock(context->shared->queueMut);
            context->searched++;

            //opendir öppnar en folder från sträng
            dir = opendir(path);
            if (dir) {
                //readdir dundrar igenom nästa fil i en folder och sparar i en struct dirent
                /* print all the files and directories within directory */
                while ((ent = readdir(dir)) != NULL) {
                    char* filename = malloc(sizeof(char) * (strlen(path) + strlen(ent->d_name))+2);
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
                    //printf ("\n (0x%ld) filename: %s\n", context->id, ent->d_name);
                    //S_ISLINK och dom andra kollar om filen är link, fil eller dir
                    if(S_ISLNK(st.st_mode)) {
                        //printf("(0x%ld) Type: Symbolic link\n", context->id);  //todo skriva ut resultat korrekt
                        //printf("(0x%ld) Path: %s\n", context->id, filename);
                        //printf(" (0x%ld) jämför filename: %s med filter: %s\n", context->id, filename, context->shared->filter);
                        if (strstr(filename, context->shared->filter) != NULL){ // todo kolla om filtret är enabled
                            printf("(0x%lx) TRÄFF: %s in %s%s\n", context->id, context->shared->filter, path, ent->d_name);
                        }
                    }
                    else if(S_ISDIR(st.st_mode)){
                        //printf ("(0x%ld) Type: Directory\n", context->id);     //todo skriva ut resultat korrekt
                        //printf("(0x%ld) filename: %s\n", context->id, filename);
                        //om de är en dir och inte är . eller .. så ska den addera till kön o bränna av en ny opendir
                        if(strcmp(ent->d_name, (char *) ".") != 0 &&
                           strcmp(ent->d_name, (char *) "..") != 0) {
                            strcat(filename, (char *) "/");
                            //printf("(0x%ld) ***köar på: %s\n", context->id, filename);
                            //printf("0x%ld %s(%d)acquire queueMut\n", context->id, __FUNCTION__, __LINE__);
                            pthread_mutex_lock(context->shared->queueMut);
                            queue_enqueue(context->shared->directories, filename);
                            //printf("0x%ld %s(%d)release queueMut\n", context->id, __FUNCTION__, __LINE__);
                            pthread_mutex_unlock(context->shared->queueMut);
                            //printf("0x%ld %s(%d)acquire condMut\n", context->id, __FUNCTION__, __LINE__);
                            pthread_mutex_lock(context->shared->condMut);
                            pthread_cond_signal(context->shared->condition);
                            //printf("0x%ld %s(%d)release condMut\n", context->id, __FUNCTION__, __LINE__);
                            pthread_mutex_unlock(context->shared->condMut);
                            //printf(" (0x%ld) jämför filename: %s med filter: %s\n", context->id, path, context->shared->filter);
                            if (strstr(filename, context->shared->filter) != NULL){ // todo kolla om filtret är enabled{
                                printf("(0x%lx) TRÄFF: %s in %s%s\n", context->id, context->shared->filter, path, ent->d_name);
                            }
                            continue;
                        }
                    }
                    else if(S_ISREG(st.st_mode)){
                        printf ("(0x%lx) Type: File\n", context->id);        //todo skriva ut resultat korrekt
                        printf(" (0x%lx) jämför filename: %s med filter: %s\n", context->id, filename, context->shared->filter);
                        if (strstr(filename, context->shared->filter) != NULL){ // todo kolla om filtret är enabled{
                            printf("(0x%lx) TRÄFF: %s in %s%s\n", context->id, context->shared->filter, path, ent->d_name);
                        }

                    }
                    free(filename);
                }
                closedir (dir);
                //dir = NULL;
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
