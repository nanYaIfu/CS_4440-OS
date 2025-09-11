#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char command[50];

    while (1) {
        printf("MiniShell> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL) {
            break; 
        }

       
        command[strcspn(command, "\n")] = '\0';

        if (strcmp(command, "exit") == 0) {
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            // child process
            char *args[] = {command, NULL};
            execvp(args[0], args);
            perror("exec failed");
            exit(1);
        } else {
            // parent process
            wait(NULL);
        }
    }
    return 0;
}
