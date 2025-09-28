// Names: Ifunanya Okafor and Andy Lim || Date: 16 Sept 2025 || Course: CS 4440-03
// Description: The purpose of this program is to achieve the compression of MyCompress, 
// but using pipes. The pipe reader opens the source file, reads it, then writes it to the write 
// end of the pipe. Then, the pipe writer reads that end, does the compression, then writes it in 
// the destination file. Please refer to comments in MyCompress for compression rules.

// Compile Build: gcc PipeCompress.c -o PipeCompress
// Run: ./MyCompress "source_filename.txt" "destination_filename.txt" 

// Libraries used
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

// This function exits program in case of error
static void die(const char *m){ perror(m); _exit(1); }

// This function loops write() until all characters are written
static void write_all(int fd, const void *buf, size_t len){
    const char *p = (const char*)buf;
    while(len){
        ssize_t w = write(fd, p, len);
        if (w < 0){ if (errno==EINTR) continue; die("write"); }
        p += (size_t)w; len -= (size_t)w;
    }
}

// This function recursively writes a positive integer as ASCII 
// if n (size_t variable) is bigger than 10
static void write_uint_recursive(int fd, size_t n){
    if (n >= 10) write_uint_recursive(fd, n/10);
    char d = (char)('0' + (n % 10));
    write_all(fd, &d, 1);
}

// This function flushes compressable strings
static void flush_run(int out_fd, char bit, size_t run){
    if (run == 0) return;
    if (run >= 16){
        char open = (bit=='1') ? '+' : '-';
        write_all(out_fd, &open, 1);
        write_uint_recursive(out_fd, run);
        write_all(out_fd, &open, 1); 
    } else {
        for (size_t i=0;i<run;i++) write_all(out_fd, &bit, 1);
    }
}

// This function is for the pipr writer. It reads from pipe, compress, write to dest file
static int writer_proc(int pipe_rd, const char *dst_path){
    int out_fd = open(dst_path, O_CREAT|O_TRUNC|O_RDWR, 0644);
    if (out_fd < 0) die("open dest");

    char cur = 0;
    size_t run = 0;

    for(;;){
        char c;
        ssize_t r = read(pipe_rd, &c, 1);
        if (r < 0){ if (errno==EINTR) continue; die("read pipe"); }
        if (r == 0) break; // EOF from pipe reader

        if (c=='0' || c=='1'){
            if (run==0){ cur=c; run=1; }
            else if (c==cur){ run++; }
            else { flush_run(out_fd, cur, run); cur=c; run=1; }
        } else {
            flush_run(out_fd, cur, run);
            cur=0; run=0;
            write_all(out_fd, &c, 1); // copy delimiter
        }
    }
    flush_run(out_fd, cur, run);

    if (close(out_fd) < 0) die("close dest");
    if (close(pipe_rd) < 0) die("close pipe rd (writer)");
    return 0;
}

// This function is for the pipe writer. It reads from src file then writes 
// the data into pipe writing end
static int reader_proc(const char *src_path, int pipe_wr){
    int in_fd = open(src_path, O_RDONLY);
    if (in_fd < 0) die("open src");

    for(;;){
        char c;
        ssize_t r = read(in_fd, &c, 1);
        if (r < 0){ if (errno==EINTR) continue; die("read src"); }
        if (r == 0) break;
        write_all(pipe_wr, &c, 1);
    }

    if (close(in_fd) < 0) die("close src");
    if (close(pipe_wr) < 0) die("close pipe wr (reader)");
    return 0;
}

// The main function; It forks the minor work of reading the file to the child,
// while the parent writes and does the important compressions.
int main(int argc, char **argv){
    if (argc != 3){
        fprintf(stderr, "Usage: %s <source_file> <dest_file>\n", argv[0]);
        return 1;
    }
    const char *src = argv[1];
    const char *dst = argv[2];

    int pfd[2];
    if (pipe(pfd) < 0) die("pipe");

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0){
        // Child = READER
        if (close(pfd[0]) < 0) die("close rd in reader");
        int rc = reader_proc(src, pfd[1]);
        _exit(rc);
    } else {
        // Parent = WRITER
        if (close(pfd[1]) < 0) die("close wr in writer");
        int wrc = writer_proc(pfd[0], dst);

        int status=0;
        (void)waitpid(pid, &status, 0);
        return wrc;
    }
}
