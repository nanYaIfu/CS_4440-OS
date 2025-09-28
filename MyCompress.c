// Names: Ifunanya Okafor and Andy Lim || Date: 19 Sept 2025 || Course: CS 4440-03
// Description: The purpose of this program is to read and make a compressed copy of
// a given text file containing only strings of 0s and 1s that are separated by new lines/spaces.
// It compresses strings of only 1s or 0s that have a length greater than or equal to 16 
// as +n+ or -n- respectively, where n = the number of 1s/0s in the string. If the string has a length
// less than 16 or isn't a 0/1, it is left as is and flushes. The program achieves this using
// system calls (read, write, open, close, fsync) for file manipulation.

// Compile Build: gcc MyCompress.c -o MyCompress
// Run: ./MyCompress "source_filename.txt" "destination_filename.txt" 

// Libraries used
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

// This function exits program in case of error
static void die(const char *m){ perror(m); exit(1); }

// This function loops write() until all characters are written
static void write_all(int fd, const void *buf, size_t len){
    const char *p = (const char*)buf;
    while(len){
        ssize_t w = write(fd, p, len);
        if (w < 0){ if (errno==EINTR) continue; die("write"); }
        p += (size_t)w; len -= (size_t)w;
    }
}

// This function writes an unsigned integer
static void write_uint(int fd, size_t n){
    char tmp[32]; // enough for n (size_t variable)
    int i = 0;
    if (n == 0) tmp[i++] = '0';
    while(n){ tmp[i++] = (char)('0' + (n % 10)); n /= 10; }
    // digits are reversed
    for (int j = i-1; j >= 0; --j) write_all(fd, &tmp[j], 1);
}

// This function flushes compressable strings
static void flush_run(int out_fd, char bit, size_t run){
    if (run == 0) return;
    if (run >= 16){
        char open = (bit=='1') ? '+' : '-';
        char close = open;
        write_all(out_fd, &open, 1);
        write_uint(out_fd, run);
        write_all(out_fd, &close, 1);
    } else {
        // write the run one byte at a time
        for (size_t i=0;i<run;i++) write_all(out_fd, &bit, 1);
    }
}

// This function is for the main compression from one file descriptor to another.
static int compress_fd(int in_fd, int out_fd){
    char cur = 0;
    size_t run = 0;
    for(;;){
        char c;
        ssize_t r = read(in_fd, &c, 1);
        if (r < 0){ if (errno==EINTR) continue; die("read"); }
        if (r == 0) break; // EOF

        if (c=='0' || c=='1'){
            if (run==0){ cur=c; run=1; }
            else if (c==cur){ run++; }
            else { flush_run(out_fd, cur, run); cur=c; run=1; }
        } else {
            flush_run(out_fd, cur, run);
            cur=0; run=0;
            write_all(out_fd, &c, 1); // copy delimiter as-is
        }
    }
    flush_run(out_fd, cur, run);
    return 0;
}

// The main function; opens the two files and calls compress_fd(), then closes them
int main(int argc, char **argv){
    if (argc!=3){ dprintf(2,"Usage: %s <source> <dest>\n", argv[0]); return 1; }
    int in_fd  = open(argv[1], O_RDONLY);
    if (in_fd  < 0) die("open src");
    int out_fd = open(argv[2], O_CREAT|O_TRUNC|O_RDWR, 0644);
    if (out_fd < 0) die("open dst");

    int rc = compress_fd(in_fd,out_fd);
    fsync(out_fd);

    close(in_fd); close(out_fd);
    return rc;
}
