// Question 5
// the input file is divided into 'num_processes' chunks of roughly equal size
// each chunk, the parent forks a child process
// the parent waits for all the children to finish then combines all the temporary files into a single output
// the parent will divide the work, manage the children, and merge the results
// the child will compress the assigned file chunk into a temporary file


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

// Compression function (placeholder - would implement actual compression algorithm)
void compress_chunk(FILE *input, FILE *output, long start_pos, long chunk_size) {
    char buffer[4096];
    size_t bytes_read;
    long remaining = chunk_size;
    
    fseek(input, start_pos, SEEK_SET);
    
    while (remaining > 0 && (bytes_read = fread(buffer, 1, 
           (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining, input)) > 0) {
        // Apply compression algorithm to buffer
        fwrite(buffer, 1, bytes_read, output);
        remaining -= bytes_read;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <output_file> <num_processes>\n", argv[0]);
        return 1;
    }
    
    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    int num_processes = atoi(argv[3]);
    
    if (num_processes <= 0) {
        fprintf(stderr, "Number of processes must be positive\n");
        return 1;
    }
    
    // Open input file and get size
    FILE *input = fopen(input_filename, "rb");
    if (!input) {
        perror("Failed to open input file");
        return 1;
    }
    
    // Get file size
    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    rewind(input);
    
    // Calculate chunk size
    long chunk_size = file_size / num_processes;
    long remainder = file_size % num_processes;
    
    // Create temporary filenames for each process
    char **temp_filenames = malloc(num_processes * sizeof(char *));
    pid_t *child_pids = malloc(num_processes * sizeof(pid_t));
    
    for (int i = 0; i < num_processes; i++) {
        temp_filenames[i] = malloc(32);
        sprintf(temp_filenames[i], "temp_chunk_%d.tmp", i);
        
        // Calculate start position and actual chunk size for this process
        long start_pos = i * chunk_size;
        long process_chunk_size = chunk_size;
        if (i == num_processes - 1) {
            process_chunk_size += remainder;  // Last process gets any remainder
        }
        
        // Fork a child process
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            FILE *chunk_output = fopen(temp_filenames[i], "wb");
            if (!chunk_output) {
                perror("Failed to create temporary output file");
                exit(1);
            }
            
            // Compress this chunk
            compress_chunk(input, chunk_output, start_pos, process_chunk_size);
            
            fclose(chunk_output);
            fclose(input);
            exit(0);
        } else if (pid < 0) {
            perror("Fork failed");
            return 1;
        } else {
            // Parent process
            child_pids[i] = pid;
        }
    }
    
    // Parent waits for all children to complete
    for (int i = 0; i < num_processes; i++) {
        int status;
        waitpid(child_pids[i], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child process %d failed\n", i);
        }
    }
    
    // Combine all temporary files into the output file
    FILE *output = fopen(output_filename, "wb");
    if (!output) {
        perror("Failed to create output file");
        return 1;
    }
    
    for (int i = 0; i < num_processes; i++) {
        FILE *temp_file = fopen(temp_filenames[i], "rb");
        if (!temp_file) {
            perror("Failed to open temporary file");
            continue;
        }
        
        char buffer[4096];
        size_t bytes_read;
        
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), temp_file)) > 0) {
            fwrite(buffer, 1, bytes_read, output);
        }
        
        fclose(temp_file);
        remove(temp_filenames[i]);  // Delete temporary file
        free(temp_filenames[i]);
    }
    
    fclose(output);
    fclose(input);
    free(temp_filenames);
    free(child_pids);
    
    printf("File processing completed.\n");
    return 0;
}
