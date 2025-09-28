#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

// Compression to Uppercase
void myCompress(char *buffer, ssize_t length) {
    for (ssize_t i = 0; i < length; i++) {
        if (buffer[i] >= 'a' && buffer[i] <= 'z') {
            buffer[i] = buffer[i] - 'a' + 'A';
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <destination_file>\n", argv[0]);
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[1]); // Close pipe writer

        int dest_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest_fd < 0) {
            perror("open destination file");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(pipefd[0], buffer, BUFFER_SIZE)) > 0) {
            myCompress(buffer, bytesRead);
            write(dest_fd, buffer, bytesRead);
        }

        close(dest_fd);
        close(pipefd[0]);
    } else {
        // Parent process
        close(pipefd[0]); // Close pipe reader

        int src_fd = open(argv[1], O_RDONLY);
        if (src_fd < 0) {
            perror("open source file");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
            write(pipefd[1], buffer, bytesRead);
        }

        close(src_fd);
        close(pipefd[1]);
        wait(NULL); 
    }

    return 0;
}
