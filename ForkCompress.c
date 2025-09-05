// Question 3
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) 
{
    if (argc != 3) 
    {
        fprintf(stderr, "Usage: %s <source_file> <dest_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
   
    if (pid < 0) 
    {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
     {
        // Child process
        // execl(path_to_program, argv[0], arg1, arg2, ..., NULL)
        execl("./MyCompress", "MyCompress", argv[1], argv[2], (char *)NULL);

        // If execl returns, there was an error
        perror("execl failed");
        exit(EXIT_FAILURE);
    } 
    
    else 
    {
        // Parent process
        int status;
        if (wait(&status) == -1) 
        {
            perror("wait failed");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status)) // true if the child terminated normally
        {
            printf("Compression finished, exit code: %d\n", WEXITSTATUS(status));
        } else 
        {
            printf("Compression process did not terminate normally.\n");
        }
    }

    return 0;
}