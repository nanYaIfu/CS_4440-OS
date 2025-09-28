// Names: Ifunanya Okafor and Andy Lim || Date: 20 Sept 2025 || Course: CS 4440-03
// Description: The purpose of this program is the most powerful out of all three shells,
// extending both MiniShell AND MoreShell by allowing pipes in arguments  for
// system-line commands. This is done using strdup() and dup2().
// See MiniShell and MoreShell for more details.

// Compile Build: gcc DupShell.c -o DupShell
// Run: ./Duphell

// Libraries used
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define LINE_MAX  4096
#define MAX_ARGS   128
#define MAX_CMDS    32

typedef struct {
    char *argv[MAX_ARGS];
    int   argc;
} Command;

static void die(const char *m){ perror(m); exit(1); }

// expansion for a single token; returns malloc'd string 
static char *expand_tilde(const char *tok){
    if (tok[0] != '~') return strdup(tok);
    const char *home = getenv("HOME");
    if (!home || !*home) return strdup(tok);
    if (tok[1] == '\0'){
        return strdup(home);
    } else if (tok[1] == '/') {
        size_t n1 = strlen(home), n2 = strlen(tok+1);
        char *out = (char*)malloc(n1 + n2 + 1);
        if (!out) die("malloc");
        memcpy(out, home, n1);
        memcpy(out + n1, tok + 1, n2 + 1);
        return out;
    }
    // ~user not implemented 
    return strdup(tok);
}

//free argv strings for all commands 
static void free_cmds(Command *cmds, int ncmds){
    for (int i=0; i<ncmds; ++i){
        for (int j=0; j<cmds[i].argc; ++j) free(cmds[i].argv[j]);
    }
}

// push an argument string (takes ownership of malloc'd s) 
static void push_arg(Command *cmd, char *s){
    if (cmd->argc >= MAX_ARGS-1){
        fprintf(stderr, "too many args\n");
        free(s);
        return;
    }
    cmd->argv[cmd->argc++] = s;
    cmd->argv[cmd->argc] = NULL;
}

// Parse one input line into commands; returns 1 on success (maybe 0 cmds), -1 on hard error 
static int parse_line(char *line, Command *cmds, int *out_ncmds){
    for (int i=0;i<MAX_CMDS;i++){ cmds[i].argc = 0; cmds[i].argv[0] = NULL; }

    int in_single = 0, in_double = 0, esc = 0;
    char tok[LINE_MAX];
    int  tlen = 0;

    int ccount = 0; // current command index 

    size_t L = strlen(line);
    // Iterate including a synthetic '\n' at end to flush last token/command 
    for (size_t i=0; ; ++i){
        int at_end = (i >= L);
        char c = at_end ? '\n' : line[i];

        if (esc){
            if (tlen >= LINE_MAX-1){ fprintf(stderr, "token too long\n"); return -1; }
            tok[tlen++] = c;
            esc = 0;
            continue;
        }
        if (!in_single && c == '\\'){ esc = 1; continue; }

        if (!in_double && c == '\''){ in_single = !in_single; continue; }
        if (!in_single && c == '"'){  in_double = !in_double; continue; }

        if (!in_single && !in_double){
            if (c == '|'){
                // finish current token if any 
                if (tlen){
                    tok[tlen] = '\0';
                    char *s = expand_tilde(tok);
                    push_arg(&cmds[ccount], s);
                    tlen = 0;
                }
                // reject empty command at pipe boundary 
                if (cmds[ccount].argc == 0){
                    fprintf(stderr, "syntax error: empty command near '|'\n");
                    *out_ncmds = 0;
                    return 1; // treat as handled line with no commands to run 
                }
                if (ccount >= MAX_CMDS-1){
                    fprintf(stderr, "too many pipeline stages\n");
                    *out_ncmds = 0;
                    return 1;
                }
                ccount++;
                continue;
            }

            if (isspace((unsigned char)c)){
                if (c == '\n'){
                    // end of line: finish token, then we’re done 
                    if (tlen){
                        tok[tlen] = '\0';
                        char *s = expand_tilde(tok);
                        push_arg(&cmds[ccount], s);
                        tlen = 0;
                    }
                    *out_ncmds = (cmds[ccount].argc == 0 && ccount == 0) ? 0 : (ccount + 1);
                    return 1;
                } else {
                    // space: token boundary 
                    if (tlen){
                        tok[tlen] = '\0';
                        char *s = expand_tilde(tok);
                        push_arg(&cmds[ccount], s);
                        tlen = 0;
                    }
                    continue;
                }
            }
        }

        // regular char (or inside quotes) 
        if (tlen >= LINE_MAX-1){ fprintf(stderr, "token too long\n"); return -1; }
        tok[tlen++] = c;
    }
}

// Execute a parsed pipeline 
static int run_pipeline(Command *cmds, int ncmds){
    if (ncmds == 0) return 0;

    // built-in 'exit' only if it's a lone command 
    if (ncmds == 1 && cmds[0].argc >= 1 && strcmp(cmds[0].argv[0], "exit") == 0){
        free_cmds(cmds, ncmds);
        exit(0);
    }

    int nPipes = (ncmds > 1) ? (ncmds - 1) : 0;
    int pipes[2 * (MAX_CMDS - 1)];
    for (int i=0; i<nPipes; ++i){
        if (pipe(&pipes[2*i]) < 0) die("pipe");
    }

    pid_t pids[MAX_CMDS];

    for (int i=0; i<ncmds; ++i){
        pid_t pid = fork();
        if (pid < 0) die("fork");

        if (pid == 0){
            // child: hook up stdin/stdout 
            if (nPipes > 0){
                if (i > 0){
                    if (dup2(pipes[2*(i-1)], STDIN_FILENO) < 0) die("dup2 in");
                }
                if (i < ncmds-1){
                    if (dup2(pipes[2*i + 1], STDOUT_FILENO) < 0) die("dup2 out");
                }
                for (int k=0; k<2*nPipes; ++k) close(pipes[k]);
            }
            execvp(cmds[i].argv[0], cmds[i].argv);
            perror("exec failed");
            _exit(127);
        }
        pids[i] = pid;
    }

    // parent: close pipes and wait 
    for (int k=0; k<2*nPipes; ++k) close(pipes[k]);

    int status = 0, rc = 0;
    for (int i=0; i<ncmds; ++i){
        if (waitpid(pids[i], &status, 0) < 0) perror("waitpid");
        if (WIFEXITED(status)) rc = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) rc = 128 + WTERMSIG(status);
    }
    return rc;
}

int main(void){
    char line[LINE_MAX];

    for (;;){
        fputs("DupShell> ", stdout);
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) break; // EOF 

        // skip blank lines 
        int only_ws = 1;
        for (char *p = line; *p; ++p) if (!isspace((unsigned char)*p)) { only_ws = 0; break; }
        if (only_ws) continue;

        Command cmds[MAX_CMDS];
        int ncmds = 0;
        int ok = parse_line(line, cmds, &ncmds);
        if (ok < 0) continue;      // hard parse error 
        if (ncmds == 0) continue;  /* empty/syntax */

        (void)run_pipeline(cmds, ncmds);
        free_cmds(cmds, ncmds);
    }
    return 0;
}
