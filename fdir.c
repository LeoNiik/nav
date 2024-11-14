#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <dirent.h>
#include <sys/wait.h>

#define MAX_LENGTH 1024


typedef struct {
    char **items;
    int size;
    int capacity;
} DA;

int readSubTree(char* path, DA *da);

void printFileInfo(const char* path, const struct dirent* entry);
int isDir(const char *);
void initDA(DA *da);
void appendDA(DA *da, char *item);



void initDA(DA *da){
    da->size = 0;
    da->capacity = 1;
    da->items = malloc(sizeof(char *));
}




//function to append a string to the dynamic array
int isDir(const char *path) {
    struct stat statbuf;
    if (lstat(path, &statbuf) != 0)
    {
        if (errno == ENOENT)
            return -2;
        perror("STAT");
        return 0;
    }

    if(S_ISDIR(statbuf.st_mode)){
        return 1;
    }else{
        return -1;
    }
}

int main(int argc , char ** argv){
    char *path = getenv("HOME");
    
    char *fzfquery = NULL;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-h") == 0) {
                printf("Usage: %s [-p path] [fzfquery]\n", argv[0]);
                printf("path is the path from where the search starts\nDefault is home folder", argv[0]);
                return 0;
            } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
                strcpy(path, argv[++i]);
                if(isDir(path) != 1){
                    printf("The path is not a valid directory\n");
                    return -1;
                }
            } else {
                fzfquery = malloc(strlen(argv[i]) + 1);
                strncpy(fzfquery, argv[i], strlen(argv[i]) + 1);
            }
        }
    }

    DA fzfargs;
    initDA(&fzfargs);
    appendDA(&fzfargs, path);    
    


    
    
    int pipefd[2];
    if(pipe(pipefd) < 0){
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) { // Child process
        // Close the read end of the pipe
        close(pipefd[0]);

        // Redirect stdout to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Read and write data
        readSubTree(path, &fzfargs);
        for (int i = 0; i < fzfargs.size; i++) {
            write(STDOUT_FILENO, fzfargs.items[i], strlen(fzfargs.items[i]));
            write(STDOUT_FILENO, "\n", 1); // Add newline between items
        }

        close(pipefd[1]); // Close the write end of the pipe
        printf("Child process finished\n");
        exit(EXIT_SUCCESS);

    } else { // Parent process
        // Close the write end of the pipe
        close(pipefd[1]);

        // Redirect stdin to the read end of the pipe
        dup2(pipefd[0], STDIN_FILENO);
            
        int status;

        usleep(5000);
        if(waitpid(pid, &status, WNOHANG) < 0){
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
            exit(EXIT_FAILURE);
        }
        
        // Execute `fzf`, reading from stdin
        if (fzfquery != NULL) {

            execlp("fzf", "fzf", "-q", fzfquery, (char *)NULL);
        }
        execlp("fzf", "fzf", (char *)NULL);

        // If execlp fails, handle error
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    return 0; 
}

int readSubTree(char* path, DA *da){
    DIR * currdir;
    char buf[MAX_LENGTH];

    if((currdir = opendir(path)) == NULL){
        
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    
    struct dirent * entry;
    int i = 0;
    while((entry = readdir(currdir)) != NULL){
        
        
        //if the entry is the current directory or the parent directory
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        //if the entry is an hidden directory
        if(strncmp(entry->d_name, ".", 1) == 0){
            continue;
        }
        
        char newPath[MAX_LENGTH] = {0};

        snprintf(newPath, sizeof(newPath), "%s/%s", path, entry->d_name);
        if ((isDir(newPath) == 1)) {
            //if it is a directory print it to the stdout
            readSubTree(newPath, da);
            char *newPathCopy = strdup(newPath);
            appendDA(da, newPathCopy);

        }
    }
    closedir(currdir);
    return 0;

}

void appendDA(DA *da, char *item){
    if(da->size == da->capacity){
        da->capacity *= 2;
        da->items = realloc(da->items, da->capacity * sizeof(char *));
    }
    da->items[da->size++] = item;
}


