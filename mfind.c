#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <getopt.h>
#include <string.h>
#include "mfind.h"
#include "queue.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>




void getDir(int argc, char **argv, int nrArg, queue* directories, char** name);
void search(char name, queue* directories);

int main(int argc, char *argv[]) {

    //variables
    int flag;
    int nrthr = 0;
    int matchSet = false;
    char* name;
    queue* directories = queue_empty();

    //parse flags in argument
    while ((flag = getopt(argc, argv, "t:p:")) != -1) {
        switch (flag) {
            case 't':
                //flagtype = *optarg;
                if(*optarg != 'd' && *optarg != 'f' && *optarg != 'l'){
                    fprintf(stderr, "Invalid parameters!\n"
                            "Usage: mfind [-t type] [-p nrthr] start1"
                            " [start2 ...] name\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                nrthr = atoi(optarg);
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
    getDir(argc, argv, optind, directories, &name);
    search((char)name, directories);//todo tråda iväg skiten nrthr ggroch snöra ihop sen
    /*printf("\ntypeFlag: %s\nnrOfThreadFlag: %d ", &type, nrthr);
    while(!queue_isEmpty(directories)){
        printf("\nDirectory: %s ", (char *) queue_front(directories));
        queue_dequeue(directories);
    }
    printf("\nsearchFor: %s", name);*/


//    p_thread
//    2 = search(argv[1]);
//
//    joina trådar
//
//    skriv resultat

}
void search(char name, queue* directories){
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    char* path;
    char* filename;

    while(!queue_isEmpty(directories)){//todo mutex på kön

        printf("\n*** %s ***\n", (char *)queue_front(directories));
        path = queue_front(directories);
        queue_dequeue(directories);
                                                //todo mutex av
        if (dir = opendir(path)) {

            /* print all the files and directories within directory */
            while ((ent = readdir(dir)) != NULL) {
                filename = malloc(strlen(path) + strlen(ent->d_name));
                strcpy(filename, path);
                strcat(filename, ent->d_name);


                if(lstat(filename, &st) != -1){ //todo lstat är röd? wierd IDE eller nån störd include som saknas
                }else{
                    perror("lstat: ");
                }
                printf ("\nfilename: %s\n", ent->d_name);
                if(S_ISLNK(st.st_mode)) {
                    printf("Type: Symbolic link\n");  //todo syla in i resultatlista (mutex av och på den)
                    printf("Path: %s\n", filename);
                }
                else if(S_ISDIR(st.st_mode)){
                    printf ("Type: Directory\n");     //todo syla in i kön(mutex) om de inte är den dir man letar då resultatlista(mutex på den)
                    printf("path: %s\n", filename);
                    if(strcmp(ent->d_name, (char *) ".") != 0 &&
                            strcmp(ent->d_name, (char *) "..") != 0){
                        strcat(filename, "/"); //todo meka med allokeringen...verkar vara wiesse
                        printf("***köar på: %s\n", filename);
                        queue_enqueue(directories, filename);
                    }
                }
                else if(S_ISREG(st.st_mode)){
                    printf ("Type: File\n");        //todo syla in i resultatlistan (mutex av och på)
                    printf("path: %s\n", filename);
                }
                //free(filename);//todo +ade på 1 i malloc...ser ut att funka men bör undersökas
            }
            closedir (dir);
            free(filename);
        } else {
            /* could not open directory */
            perror("opendir: ");
        }
    }
}

void getDir(int argc, char **argv, int nrArg, queue* directories, char** name){

    for(; nrArg < (argc-1); nrArg++){
        printf("%s\n", argv[nrArg]);
        queue_enqueue(directories, argv[nrArg]);
    }
    *name = argv[nrArg];
}