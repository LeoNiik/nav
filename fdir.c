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

const char *argp_program_version = "1.0";
const char *argp_program_bug_address = "leonardonicoletta32@gmail.com";
static char doc[] = "Shell utility to navigate through directories";
static char args_doc[] = "";

int readSubTree(char* path);

int isDir(const char *);


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
        if(access(arguments.path, R_OK) == F_OK){
            readSubTree(arguments.path);
            printf("%s\n", arguments.path);
        }else{
            perror("specified path is not readable by this user");
        }
        usleep(500);
        
        close(pipefd[1]); // Close the write end of the pipe
        exit(EXIT_SUCCESS);

    } else { // Parent process
        
        // Close the write end of the pipe
        close(pipefd[1]);
        // Redirect stdin to the read end of the pipe
        dup2(pipefd[0], STDIN_FILENO);
            
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

int readSubTree(char* path){
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

            //check if the dir is readable for current user
            if(access(newPath, R_OK) == F_OK){
                readSubTree(newPath);
                printf("%s\n", newPath);
            }
        }
    }
    closedir(currdir);
    return 0;

}



