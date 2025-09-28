// Names: Ifunanya Okafor and Andy Lim || Date: 20 Sept 2025 || Course: CS 4440-03
// Description: The purpose of this program is to mimic a basic terminal (aka a shell)
// that can handle system commands with no arguments by prompting a system command 
// from the user, reading the command from the input, then executing it. 

// Compile Build: gcc MiniShell.c -o MiniShell
// Run: ./MiniShell

// Libraries used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

// This function resolves an issue I kept running while prototyping this, where I used
// to run into errors due to the trailing whitespace. This function trims and cuts off
// the "trailing whitespace" found in C strings.
static void trim_inplace(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    *end = '\0';
    if (start != s) memmove(s, start, (size_t)(end - start) + 1);
}

// The main function loops the shell and forks any valid commands to a child process
int main(void) {
    char command[50];

    for (;;) {
        printf("MiniShell> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) break;

        command[strcspn(command, "\n")] = '\0'; // strip newline
        trim_inplace(command);

        if (command[0] == '\0') continue;          // skip empty input
        if (strcmp(command, "exit") == 0) break;    // exits the shell

        // Enforce "argument-less" commands 
        if (strchr(command, ' ') != NULL) {
            fprintf(stderr, "Arguments are not supported (attempted: %s)\n", command);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
        } else if (pid == 0) {
            // child process
            char *args[] = { command, NULL };
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
