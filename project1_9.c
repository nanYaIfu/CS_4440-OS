// question 9

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_SIZE 4096

typedef struct {
    const char *input_filename;
    char temp_filename[32];
    long start_pos;
    long chunk_size;
    int thread_id;
} ThreadData;

// Placeholder compression function (currently just copies data)
// Replace with your Problem 1 compression logic if required
void *compress_chunk(void *arg) {
    ThreadData *data = (ThreadData *)arg;

    FILE *input = fopen(data->input_filename, "rb");
    if (!input) {
        perror("Failed to open input file");
        pthread_exit((void *)1);
    }

    FILE *output = fopen(data->temp_filename, "wb");
    if (!output) {
        perror("Failed to open temporary output file");
        fclose(input);
        pthread_exit((void *)1);
    }

    fseek(input, data->start_pos, SEEK_SET);

    char buffer[BUFFER_SIZE];
    long remaining = data->chunk_size;

    while (remaining > 0) {
        size_t to_read = (remaining > BUFFER_SIZE) ? BUFFER_SIZE : remaining;
        size_t bytes_read = fread(buffer, 1, to_read, input);
        if (bytes_read == 0) break;

        // Replace with compression logic:
        fwrite(buffer, 1, bytes_read, output);

        remaining -= bytes_read;
    }

    fclose(input);
    fclose(output);

    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <output_file> <num_threads>\n", argv[0]);
        return 1;
    }

    const char *input_filename = argv[1];
    const char *output_filename = argv[2];
    int num_threads = atoi(argv[3]);

    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        return 1;
    }

    // Get file size
    FILE *input = fopen(input_filename, "rb");
    if (!input) {
        perror("Failed to open input file");
        return 1;
    }
    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fclose(input);

    long chunk_size = file_size / num_threads;
    long remainder = file_size % num_threads;

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *thread_data = malloc(num_threads * sizeof(ThreadData));

    // Launch threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].input_filename = input_filename;
        sprintf(thread_data[i].temp_filename, "thread_chunk_%d.tmp", i);
        thread_data[i].start_pos = i * chunk_size;
        thread_data[i].chunk_size = (i == num_threads - 1) ? (chunk_size + remainder) : chunk_size;
        thread_data[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, compress_chunk, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Combine temporary files into final output
    FILE *output = fopen(output_filename, "wb");
    if (!output) {
        perror("Failed to create output file");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    for (int i = 0; i < num_threads; i++) {
        FILE *temp_file = fopen(thread_data[i].temp_filename, "rb");
        if (!temp_file) {
            perror("Failed to open temporary file");
            continue;
        }

        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), temp_file)) > 0) {
            fwrite(buffer, 1, bytes_read, output);
        }

        fclose(temp_file);
        remove(thread_data[i].temp_filename);  // delete temp file
    }

    fclose(output);
    free(threads);
    free(thread_data);

    printf("Parallel compression using threads completed successfully.\n");
    return 0;
}
