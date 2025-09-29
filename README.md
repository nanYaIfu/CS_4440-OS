# Project 1
Table of Contents
------------
1) MyCompress.c
   - Purpose: Compress a text file of 0s and/or 1s; strings with length >= 16 are encoded as +n+ (ones) or -n- (zeros), where n is the length of the string; other chars are left the same
   - Compile: gcc MyCompress.c -o MyCompress
   - Run:     ./MyCompress <source_file> <destination_file>

2) MyDecompress.c
   - Purpose: Decompress files produced by MyCompress, expanding +n+ to n ones and -n- to n zeros; other chars stay the same.
   - Compile: gcc MyDecompress.c -o MyDecompress
   - Run:     ./MyDecompress <compressed_file> <decompressed_file>

3) ForkCompress.c
   - Purpose: Two-process fork version of the compressor (MyCompress) where the child will do everything (child labor) and the parent will wait until    child process is done and report whether or not it succeeded.
   - Compile: gcc ForkCompress.c -o ForkCompress
   - Run:     ./ForkCompress <source_file> <destination_file>

4) PipeCompress.c
   - Purpose: Two-process pipe version of the compressor (MyCompress) to split the work of reading and writing; reader child streams source into a pipe; writer process compresses and writes to destination.
   - Compile: gcc PipeCompress.c -o PipeCompress
   - Run:     ./PipeCompress <source_file> <destination_file>

5) ParFork.c
   - Purpose: A concurrent version of the compressor (MyCompress) that divides the file into an n number of equal chunks that are then shared among an n number of processes to work on separately, then converge together their work when done.
   - Compile: gcc ParFork.c -o ParFork
   - Run:     ./ParFork <source_file> <destination_file>

6) MiniShell.c
   - Purpose: Mini shell that executes argument-less commands using exec, wait, etc., including exit.
   - Compile: gcc MiniShell.c -o MiniShell
   - Run:     ./MiniShell

7) MoreShell.c
   - Purpose: Similar to Minishell, but accepts arguments in commands using parsing.
   - Compile: gcc MoreShell.c -o MoreShell
   - Run:     ./MoreShell

8) DupShell.c
   - Purpose: Similar to Minishell, but can run pipelines in commands using pipe() and dup2()
   - Compile: gcc DupShell.c -o DupShell
   - Run:     ./DupShell
  
9) ParThread.c
   - Purpose: Multi-threaded version of the concurrent compressor (ParFork)
   - Compile: gcc ParThread.c -o ParThread -lpthread
   - Run:     ./ParThread <source_file> <destination_file>

Other(s)/Notes
--------------------
- test_runs.txt — Typescript file showing the working of all the programs for correct input as well as graceful exit on error input.
- PRJ1README - Glossary of Project 1

