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
#include <argp.h>
#include <stdbool.h>
#include <linux/limits.h>


//what is the max limit of paths in linux?
//what is the max limit of paths in windows?



const char *argp_program_version = "1.0";
const char *argp_program_bug_address = "leonardonicoletta32@gmail.com";
static char doc[] = "Shell utility to navigate through directories";
static char args_doc[] = "";

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



static struct argp_option options[] = { 
    
    { "path", 'p', "PATH", 0, "searches from the specified path (default $HOME)"},
    { "query", 'q', "QUERY", 0, "Inputs the query in the fzf search"},    
    { 0 } 
};

struct arguments
{
	char *query;                /* table name and input file*/
	char *path;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
    case 'p': arguments->path = arg; break;
    case 'q': arguments->query = arg; break;
    case ARGP_KEY_ARG: return 0;
    default: return ARGP_ERR_UNKNOWN;
    }   
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };


int main(int argc , char ** argv){

    //professional input argument parser
    struct arguments arguments;

    arguments.query = NULL;
    arguments.path = getenv("HOME");


    argp_parse(&argp, argc, argv, 0, 0, &arguments);
    


    DA fzfargs;
    initDA(&fzfargs);
    appendDA(&fzfargs, arguments.path);    
    

    
    
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

        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);


        // Read and write data
        readSubTree(arguments.path, &fzfargs);

        //put all the in items in a single string
        char *allPaths = malloc(fzfargs.size * PATH_MAX);

        for(int i = 0; i < fzfargs.size; i++){
            strcat(allPaths, fzfargs.items[i]);
            strcat(allPaths, "\n");
        }

        write(pipefd[1], allPaths, strlen(allPaths));
        if (write(pipefd[1], "\0", 1) < 0) {
            perror("write");
            return -1;
        }


        close(pipefd[1]); // Close the write end of the pipe
        exit(EXIT_SUCCESS);

    } else { // Parent process
        
        // Close the write end of the pipe
        close(pipefd[1]);
        // Redirect stdin to the read end of the pipe
        dup2(pipefd[0], STDIN_FILENO);

            
        int status;

        while (waitpid(pid, &status, WNOHANG) < 0) {
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_SUCCESS) {
            exit(EXIT_FAILURE);
        }
        
        // Execute `fzf`, reading from stdin
        if (arguments.query != NULL) {
            
            execlp("fzf", "fzf", "-q", arguments.query, (char *)NULL);
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
    char buf[PATH_MAX];

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
        
        char newPath[PATH_MAX] = {0};

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


