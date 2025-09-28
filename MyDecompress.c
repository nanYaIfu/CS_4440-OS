// Names: Ifunanya Okafor and Andy Lim || Date: 19 Sept 2025 || Course: CS 4440-03
// Description: The purpose of this program is to reverse the effects of MyCompress.c by 
// reading a given text file that has been compressed and makes a decompressed copy of it.
// For every +n+ or -n- it reads, it will write a string of length n of 1s or 0s respectively.
// Everything else that isn't in that format is left as is in the decompressed copy. Once again,
// this program only uses system calls.

// Compile Build: gcc MyDecompress.c -o MyDecompress
// Run: ./MyCompress "source_filename.txt" "destination_filename.txt" 

// Libraries used
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

// This function exits program in case of error
static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// This function loops write() until all characters are written
static void write_all(int fd, const void *buf, size_t len) {
    const char *p = (const char*)buf;
    while (len > 0) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR) continue;
            die("write");
        }
        p += (size_t)w;
        len -= (size_t)w;
    }
}

// This function writes an n number of copies of characters to fd.
static void write_repeat(int fd, char ch, size_t count) {
    if (count == 0) return;
    // Uses a buffer to limit write() calls for massive runs
    char chunk[65536];
    memset(chunk, (unsigned char)ch, sizeof(chunk));
    while (count > 0) {
        size_t n = count < sizeof(chunk) ? count : sizeof(chunk);
        write_all(fd, chunk, n);
        count -= n;
    }
}

// This function attempts to parse a token starting with '+' or '-' from in_fd.
// If successful, emits the corresponding run to out_fd and returns 1.
// If error occurs, writes the literal characters it has consumed
// back out and returns 0.
static int try_parse_token(int in_fd, int out_fd, char sign) {
    // Buffer to hold characters we read while attempting to parse.
    char tbuf[64];
    size_t tlen = 0;

    tbuf[tlen++] = sign;

    // Reading the digits
    size_t num = 0;
    int have_digit = 0;
    for (;;) {
        char c;
        ssize_t r = read(in_fd, &c, 1);
        if (r < 0) {
            if (errno == EINTR) continue;
            die("read");
        }
        if (r == 0) {
            // EOF
            write_all(out_fd, tbuf, tlen);
            return 0;
        }

        if (c >= '0' && c <= '9') {
            have_digit = 1;
            // Cap at SIZE_MAX/10 to prevent overflowing, as seen in previous prototype attempts
            if (num > (SIZE_MAX / 10)) {
                // Emit what we've buffered and the current char, then stop.
                write_all(out_fd, tbuf, tlen);
                write_all(out_fd, &c, 1);
                return 0;
            }
            num = num * 10 + (size_t)(c - '0');
            if (tlen < sizeof(tbuf)) tbuf[tlen++] = c;
            else {
                // If too long for tbuf, treat as error
                write_all(out_fd, tbuf, tlen);
                write_all(out_fd, &c, 1);
                return 0;
            }
            continue;
        }

        // Non-digits must be matching closing sign to be valid
        if (c == sign && have_digit) {
            char bit = (sign == '+') ? '1' : '0';
            write_repeat(out_fd, bit, num);
            return 1;
        } else {
            // Error; write out buffered literal + this char, as-is
            write_all(out_fd, tbuf, tlen);
            write_all(out_fd, &c, 1);
            return 0;
        }
    }
}

// This funciton is for the main decompression from one file descriptor to another.
static int decompress_fd(int in_fd, int out_fd) {
    for (;;) {
        char c;
        ssize_t r = read(in_fd, &c, 1);
        if (r < 0) {
            if (errno == EINTR) continue;
            die("read");
        }
        if (r == 0) break; // EOF

        if (c == '+' || c == '-') {
            // Try to parse a +n+ (ones) or -n- (zeros) 
            if (!try_parse_token(in_fd, out_fd, c)) {
                // Already emitted the literal inside try_parse_token upon failure
            }
        } else {
            // Copy everything else as-is 
            write_all(out_fd, &c, 1);
        }
    }
    return 0;
}

// The main function; opens the files, calls the decompress_fd, then closes
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <compressed_in> <decompressed_out>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int in_fd = open(argv[1], O_RDONLY);
    if (in_fd < 0) die("open input");

    int out_fd = open(argv[2], O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (out_fd < 0) {
        close(in_fd);
        die("open output");
    }

    int rc = decompress_fd(in_fd, out_fd);
    fsync(out_fd);

    if (close(in_fd) < 0) die("close input");
    if (close(out_fd) < 0) die("close output");

    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
