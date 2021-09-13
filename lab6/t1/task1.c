#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LineParser.c"
#include <sys/wait.h>

int is_debug_mode = 0;

void mypipeline()
{
    int dup_read_end, dup_write_end, pid1, pid2;
    int fd[2];
    //1 - creatig a pipe
    if (pipe(fd) == -1)
    {
        printf("ERROR: couldn't open the pipe\n");
    }
    else
    {
        // 2 - forking a child process
        if (is_debug_mode == 1)
        {
            fprintf(stderr, "(parent_process>forking…)\n");
        }
        pid1 = fork();
        if (pid1 < 0)
        {
            printf("ERROR: couldn't execute fork!\n");
            return;
        }
        // 3 - child process
        if (pid1 == 0)
        {
            if (is_debug_mode == 1)
            {
                fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");
            }
            // 3-a
            close(1); // 1 is the fd of stdout
            // 3-b
            dup_write_end = dup(fd[1]);
            // 3-c
            close(fd[1]);
            // 3-d
            char *ls_args[] = {"ls", "-l", NULL};
            if (is_debug_mode == 1)
            {
                fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");
            }
            execvp("ls", ls_args);
            perror("ERROR executing ls -l\n");
            _exit(0);
        }
        else
        {
            if (is_debug_mode == 1)
            {
                fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);
                fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
            }
            // 4
            close(fd[1]); // when commanted out, the shell waits for more input from the user, the program doesn't end
            //because we don't close the write - end fd
            // 5
            if (is_debug_mode == 1)
            {
                fprintf(stderr, "(parent_process>forking…)\n");
            }
            pid2 = fork();
            if (pid2 < 0)
            {
                printf("ERROR: couldn't execute fork!");
                return;
            }
            // 6
            else if (pid2 == 0)
            {
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
                }
                // 6-a
                close(0); // 0 is the fd of stdin
                // 6-b
                dup_read_end = dup(fd[0]);
                // 6-c
                close(fd[0]);
                // 6-d
                char *tail_args[] = {"tail", "-n", "2", NULL};
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");
                }
                execvp("tail", tail_args);
                perror("ERROR executing ls -tail -n 2\n");
                _exit(0);
            }
            else
            {
                // 7
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);
                    fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
                }
                close(fd[0]); //when commented out - doesn't make any difference beacuse it's the read file and not the write - end
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
                }
                // 8
                waitpid(pid1, NULL, 0); // waits for the 1st child to finish
                // when commented out - doesn't perform the commands because the parent ends the program running before the child has a
                // chance to perform it's code part
                // and print some of the debug prints after the program finish
                waitpid(pid2, NULL, 0); // waits for the 2nd child to finish
                // when commanted out with step 4 - the program doesn't wait for more input and
                // terminates immediatly at the end of the process, without performing the cmd commands -
                // the children dont have the chance to perform their program
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(parent_process>exiting…)\n");
                }
                _exit(0);
            }
        }
    }
}

int main(int argc, char **argv)
{
    for (size_t i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "-d", 2) == 0)
            is_debug_mode = 1;
    }
    mypipeline();
    return 0;
}