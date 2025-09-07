
#include <unistd.h>
#include <fcntl.h>   
#include <stdlib.h>   
#include <stddef.h>   

#define RBUFSZ 8192
#define WBUFSZ 4096

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


static void emit_bits(int fd, char bit, size_t count) {
    if (count == 0) return;
    char chunk[WBUFSZ];
    for (size_t i = 0; i < sizeof chunk; ++i) chunk[i] = bit;
    while (count) {
        size_t n = count < sizeof chunk ? count : sizeof chunk;
        write_all(fd, chunk, n);
        count -= n;
    }
}

static int decompress_stream(int src_fd, int dst_fd) {
    char buf[RBUFSZ];

  
    int in_code = 0;          
    size_t number = 0;       
    int have_digit = 0;     
    char litbuf[64];
    size_t litlen = 0;

    for (;;) {
        ssize_t n = read(src_fd, buf, sizeof buf);
        if (n < 0) return -1;
        if (n == 0) break;

        for (ssize_t i = 0; i < n; ++i) {
            char c = buf[i];

            if (!in_code) {
                if (c == '+' || c == '-') {
                    in_code = 1;
                    sign = c;
                    number = 0;
                    have_digit = 0;
                    litlen = 0;
                    if (litlen < (sizeof litbuf)) litbuf[litlen++] = c;
                } else {
                    write_all(dst_fd, &c, 1);
                }
            } else {
             
                if (c >= '0' && c <= '9') {
                    have_digit = 1;
                    number = number * 10 + (size_t)(c - '0');
                    if (litlen < (sizeof litbuf)) litbuf[litlen++] = c;
                } else if (c == sign && have_digit) {
                    emit_bits(dst_fd, (sign == '+') ? '1' : '0', number);
                    in_code = 0;
                    sign = 0;
                    number = 0;
                    have_digit = 0;
                    litlen = 0;
                } else {
                    if (litlen) write_all(dst_fd, litbuf, litlen);
                    write_all(dst_fd, &c, 1);
                    in_code = 0;
                    sign = 0;
                    number = 0;
                    have_digit = 0;
                    litlen = 0;
                }
            }
        }
    }
    if (in_code && litlen) write_all(dst_fd, litbuf, litlen);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        const char u[] = "Usage: decompredded <source_compressed.txt> <dest_bits.txt>\n";
        write(STDERR_FILENO, u, sizeof u - 1);
        return 1;
    }

    int s = open(argv[1], O_RDONLY);
    if (s < 0) die();

    int d = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (d < 0) die();

    if (decompress_stream(s, d) < 0) die();

    if (close(s) < 0) die();
    if (close(d) < 0) die();
    return 0;
}
