
#include <unistd.h>  
#include <fcntl.h>    
#include <stdlib.h>   
#include <stddef.h>   

#define RBUFSZ 8192
#define RUN_THRESHOLD 16

static void die(void) {
    const char msg[] = "error\n";
    write(STDERR_FILENO, msg, sizeof msg - 1);
    _exit(EXIT_FAILURE);
}

static void write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char *)buf;
    while (len) {
        ssize_t n = write(fd, p, len);
        if (n < 0) die();
        p += (size_t)n;
        len -= (size_t)n;
    }
}


static int utoa_dec(size_t n, char *out) {
    char tmp[32];
    int i = 0;
    if (n == 0) { out[0] = '0'; return 1; }
    while (n) { tmp[i++] = (char)('0' + (n % 10)); n /= 10; }
    for (int j = 0; j < i; ++j) out[j] = tmp[i - 1 - j];
    return i;
}

static void flush_run(int dst_fd, char bit, size_t len) {
    if (!bit || len == 0) return;

    if (len >= RUN_THRESHOLD) {
        // NOTE:  +N+ for ones, -N- for zeros
        char buf[40];
        int k = 0;
        buf[k++] = (bit == '1') ? '+' : '-';
        k += utoa_dec(len, buf + k);
        buf[k++] = buf[0]; 
        write_all(dst_fd, buf, (size_t)k);
    } else {
       
        for (size_t i = 0; i < len; ++i) write_all(dst_fd, &bit, 1);
    }
}

static int compress_stream(int src_fd, int dst_fd) {
    char buf[RBUFSZ];
    char cur = 0;        
    size_t run = 0;

    for (;;) {
        ssize_t n = read(src_fd, buf, sizeof buf);
        if (n < 0) return -1;
        if (n == 0) break;

        for (ssize_t i = 0; i < n; ++i) {
            char c = buf[i];

            if (c == '0' || c == '1') {
                if (cur == 0) { cur = c; run = 1; }
                else if (c == cur) { run++; }
                else {
                    flush_run(dst_fd, cur, run);
                    cur = c; run = 1;
                }
            } else if (c == ' ' || c == '\n' || c == '\r') {
              
                flush_run(dst_fd, cur, run);
                cur = 0; run = 0;
                write_all(dst_fd, &c, 1);
            } else {
                
                flush_run(dst_fd, cur, run);
                cur = 0; run = 0;
                write_all(dst_fd, &c, 1);
            }
        }
    }

        flush_run(dst_fd, cur, run);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        const char u[] = "Usage: MyCompress <source_bits.txt> <dest_compressed.txt>\n";
        write(STDERR_FILENO, u, sizeof u - 1);
        return 1;
    }

    int s = open(argv[1], O_RDONLY);
    if (s < 0) die();

    int d = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (d < 0) die();

    if (compress_stream(s, d) < 0) die();

    if (close(s) < 0) die();
    if (close(d) < 0) die();
    return 0;
}
