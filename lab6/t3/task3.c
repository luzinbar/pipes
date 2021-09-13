#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "LineParser.c"
#include <sys/wait.h>

#define historyMaxLength 10

int is_debug_mode = 0;
int is_cd_mode = 0;
char *history_array[historyMaxLength];
int history_free_idx = 0;

void change_input_output(cmdLine *pcmd)
{
    if (pcmd->inputRedirect != NULL)
    {
        close(0); // 0 is the fd of stdin, if the inputRedirect is not null we need to close stdin and use this input redirection
        fopen(pcmd->inputRedirect, "r");
    }
    if (pcmd->outputRedirect != NULL)
    {
        close(1); // 1 is the fd of stdout, if the outputRedirect is not null we need to close stdout and use this output redirection
        fopen(pcmd->outputRedirect, "w");
    }
}

void pipedcommand(cmdLine *pCmdLine)
{
    cmdLine *first_cmd = pCmdLine;
    cmdLine *secnd_cmd = pCmdLine->next;
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
            change_input_output(first_cmd);
            if (is_debug_mode == 1)
            {
                fprintf(stderr, "(child1>going to execute cmd: ");
                for (int i = 0; i < first_cmd->argCount; i++)
                {
                    fprintf(stderr, "%s ", first_cmd->arguments[i]);
                }
                fprintf(stderr, ")\n");
            }
            execvp(first_cmd->arguments[0], first_cmd->arguments);
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
                change_input_output(secnd_cmd);
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(child2>going to execute cmd: ");
                    for (int i = 0; i < secnd_cmd->argCount; i++)
                    {
                        fprintf(stderr, "%s ", secnd_cmd->arguments[i]);
                    }
                    fprintf(stderr, ")\n");
                }
                execvp(secnd_cmd->arguments[0], secnd_cmd->arguments);
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
                close(fd[0]); //when commented out - doesn't make any difference
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
                }
                // 8
                waitpid(pid1, NULL, 0); // waits for the 1st child to finish
                // when commented out - doesn't perform the commands
                // and print some of the debug prints after the program finish
                waitpid(pid2, NULL, 0); // waits for the 2nd child to finish
                // when commanted out with step 4 - the program doesn't wait for more input and
                // terminates immediatly at the end of the process, without performing the cmd commands
                if (is_debug_mode == 1)
                {
                    fprintf(stderr, "(parent_process>exiting…)\n");
                }
            }
        }
    }
}

/* receives a parsed line and invokes the program specified in the cmdLine using the proper system call */
void execute(cmdLine *pCmdLine)
{
    // addition to lab6 - task 2
    if (pCmdLine->next != NULL)
    {
        pipedcommand(pCmdLine);
    }
    else
    {
        char current_directory[PATH_MAX];
        printf("current directory: %s\n\n", getcwd(current_directory, PATH_MAX));
        if (is_debug_mode == 1)
        {
            fprintf(stderr, "PID is: %d\n", getpid());
            fprintf(stderr, "Executing command is: %s\n\n", pCmdLine->arguments[0]);
        }

        if (is_cd_mode == 1)
        {
            is_cd_mode = 0;
            if (chdir(pCmdLine->arguments[0]) == -1)
                perror("ERROR changing directory");
            else
            {
                char current_directory[PATH_MAX];
                printf("directory been changed, current directory: %s\n", getcwd(current_directory, PATH_MAX));
            }
        }
        else
        {
            pid_t pid = fork();
            if (pid == 0) //child's process
            {
                // lab 6 addition
                change_input_output(pCmdLine);
                execvp(pCmdLine->arguments[0], pCmdLine->arguments);
                perror("ERROR - couldn't execute the file :(\n");
                exit(1);
            }
            else if (pid > 0 && pCmdLine->blocking == 1) // parent's process
            {
                waitpid(pid, NULL, 0);
            }
        }
    }
}

void add_to_history(char *input)
{
    if (history_free_idx < historyMaxLength)
    {
        history_array[history_free_idx] = malloc(strlen(input) * sizeof(char));
        strcpy(history_array[history_free_idx], input);
        history_free_idx++;
    }
    else
    {
        for (int i = 1; i < historyMaxLength; i++)
        {
            free(history_array[i - 1]);
            history_array[i - 1] = malloc(strlen(history_array[i]));
            strcpy(history_array[i - 1], history_array[i]);
        }
        free(history_array[history_free_idx - 1]);
        history_array[history_free_idx - 1] = malloc(strlen(input));
        strcpy(history_array[history_free_idx - 1], input);
    }
}
void print_history()
{
    for (int i = 0; i < history_free_idx; i++)
    {
        printf("#%d", i);
        printf(" %s", history_array[i]);
    }
}

void free_history()
{
    for (int i = 0; i < history_free_idx; i++)
    {
        free(history_array[i]);
    }
}

int main(int argc, char **argv)
{
    int max_bytes = 2048;
    cmdLine *parsed_line;
    /* checks is we're in debug mode*/
    char user_line[PATH_MAX];
    for (size_t i = 0; i < argc; i++)
    {
        if (strncmp(argv[i], "-d", 2) == 0)
            is_debug_mode = 1;
    }
    char current_directory[PATH_MAX];
    int should_execute;
    while (1)
    {
        parsed_line = NULL;
        should_execute = 0;
        printf("\nplease insert your command:\n");

        /* 2 - Reads a line from the "user" */
        fgets(user_line, max_bytes, stdin);
        if (strncmp(user_line, "!", 1) != 0)
            add_to_history(user_line);
        if (strncmp(user_line, "quit", 4) == 0)
        {
            free_history();
            break;
        }

        if (strncmp(user_line, "!", 1) == 0)
        {
            int indx = (user_line[1] - '0');
            if (indx < 0 || indx >= history_free_idx)
            {
                printf("ERROR - command index doesn't exist in the history\n");
                should_execute = 1;
            }
            else
            {
                strcpy(user_line, history_array[indx]);
                add_to_history(user_line);
            }
        }

        if (strncmp(user_line, "history", 7) == 0)
        {
            print_history();
            should_execute = 1;
        }

        else if (strncmp(user_line, "cd", 2) == 0)
        {
            /* skips the 'cd ' */
            is_cd_mode = 1;
            parsed_line = parseCmdLines(user_line + 3);
        }
        else
        {
            parsed_line = parseCmdLines(user_line);
        }
        if (should_execute == 0)
        {
            execute(parsed_line);
        }
        if (parsed_line != NULL)
        {
            freeCmdLines(parsed_line);
        }
    }
    return 0;
}