
// Names: Ifunanya Okafor and Andy Lim || Date: 20 Sept 2025 || Course: CS 4440-03
// Description: The purpose of this program extends that of MiniShell by allowing arguments in 
// system-line commands. This is achieved by parsing the user's inputted command, which supports
// quotes, spaces, etc andd implementing a basic ~ expansion. See MiniShell for more details.

// Compile Build: gcc MoreShell.c -o More\Shell
// Run: ./MoreShell

// Libraries used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#define LINE_MAX_LEN 1024

// This function expands leading ~ to $HOME for a single command
static char *expand_tilde(const char *tok) {
    if (tok[0] != '~') return strdup(tok);
    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') return strdup(tok); // no changes
    if (tok[1] == '\0') { // "~"
        size_t n = strlen(home);
        char *out = (char*)malloc(n + 1);
        if (!out) return NULL;
        memcpy(out, home, n + 1);
        return out;
    } else if (tok[1] == '/') { // for example, "~/path"
        size_t n1 = strlen(home), n2 = strlen(tok + 1);
        char *out = (char*)malloc(n1 + n2 + 1);
        if (!out) return NULL;
        memcpy(out, home, n1);
        memcpy(out + n1, tok + 1, n2 + 1); // include '/'
        return out;
    }
    // else it leaves as-is
    return strdup(tok);
}

// This function is a parser that splits by whitespace, supports quotes and "\" escapes
static int parse_line(const char *line, char ***argv_out) {
    size_t cap = 8, argc = 0;
    char **argv = (char**)malloc(cap * sizeof(char*));
    if (!argv) return -1;

    size_t bufcap = 64, buflen = 0;
    char *buf = (char*)malloc(bufcap);
    if (!buf) { free(argv); return -1; }

    int in_single = 0, in_double = 0;
    const unsigned char *p = (const unsigned char*)line;

    #define FLUSH_TOKEN() do { \
        if (buflen > 0) { \
            buf[buflen] = '\0'; \
            char *expanded = expand_tilde(buf); \
            if (!expanded) expanded = strdup(buf); \
            if (argc == cap) { \
                cap *= 2; \
                char **tmp = (char**)realloc(argv, cap * sizeof(char*)); \
                if (!tmp) { /* free cleaning*/ \
                    for (size_t i=0;i<argc;i++) free(argv[i]); \
                    free(argv); free(buf); return -1; \
                } \
                argv = tmp; \
            } \
            argv[argc++] = expanded; \
            buflen = 0; \
        } \
    } while(0)

    while (*p) {
        unsigned char c = *p++;

        if (in_single) {
            if (c == '\'') { in_single = 0; }
            else {
                if (buflen + 1 >= bufcap) { bufcap *= 2; buf = (char*)realloc(buf, bufcap); if (!buf) return -1; }
                buf[buflen++] = (char)c;
            }
            continue;
        }
        if (in_double) {
            if (c == '"') { in_double = 0; }
            else if (c == '\\' && *p) {
                // allow escaping inside double quotes
                if (buflen + 1 >= bufcap) { bufcap *= 2; buf = (char*)realloc(buf, bufcap); if (!buf) return -1; }
                buf[buflen++] = (char)(*p++);
            } else {
                if (buflen + 1 >= bufcap) { bufcap *= 2; buf = (char*)realloc(buf, bufcap); if (!buf) return -1; }
                buf[buflen++] = (char)c;
            }
            continue;
        }

        // not in quotes
        if (isspace(c)) {
            FLUSH_TOKEN();
            continue;
        }
        if (c == '\'') { in_single = 1; continue; }
        if (c == '"')  { in_double = 1; continue; }

        if (c == '\\' && *p) {
            // backslash escape
            if (buflen + 1 >= bufcap) { bufcap *= 2; buf = (char*)realloc(buf, bufcap); if (!buf) return -1; }
            buf[buflen++] = (char)(*p++);
        } else {
            if (buflen + 1 >= bufcap) { bufcap *= 2; buf = (char*)realloc(buf, bufcap); if (!buf) return -1; }
            buf[buflen++] = (char)c;
        }
    }
    // EOL
    FLUSH_TOKEN();

    free(buf);
    // terminatese argv with NULL
    if (argc == cap) {
        char **tmp = (char**)realloc(argv, (cap+1) * sizeof(char*));
        if (!tmp) { for (size_t i=0;i<argc;i++) free(argv[i]); free(argv); return -1; }
        argv = tmp;
    }
    argv[argc] = NULL;
    *argv_out = argv;
    return (int)argc;
    #undef FLUSH_TOKEN
}

// This funtion frees argv
static void free_argv(char **argv) {
    if (!argv) return;
    for (size_t i=0; argv[i]; i++) free(argv[i]);
    free(argv);
}

// The main function that gets the system command from user, strips the new line
// and any whitespace, parses it, then forrks it to run the command
int main(void) {
    char line[LINE_MAX_LEN];

    for (;;) {
        printf("MoreShell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break;

        // strip trailing newline
        line[strcspn(line, "\n")] = '\0';


        // skip if only whitespace
        int only_ws = 1;
        for (char *t = line; *t; ++t) { if (!isspace((unsigned char)*t)) { only_ws = 0; break; } }
        if (only_ws) continue;

        char **argv = NULL;
        int argc = parse_line(line, &argv);
        if (argc < 0) {
            fprintf(stderr, "parse error\n");
            continue;
        }
        if (argc == 0) { free_argv(argv); continue; }

        if (strcmp(argv[0], "exit") == 0) { free_argv(argv); break; }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            free_argv(argv);
            continue;
        } else if (pid == 0) {
            execvp(argv[0], argv);
            perror("exec failed");
            _exit(127);
        } else {
            int status = 0;
            if (waitpid(pid, &status, 0) < 0) perror("waitpid");
            free_argv(argv);
        }
    }
    return 0;
}
