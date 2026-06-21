#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    int pipefd[2];
    char buf[10];
    const char *msg[] = {"aaaaa", "bbbbbbbbb", ""};

    if (pipe(pipefd) == -1)
        err(EXIT_FAILURE, "pipe");

    pid_t childPIDS[2];
    for (int i = 0; i < 2; i++)
    {
        if ((childPIDS[i] = fork()) == -1)
        {
            perror("Can't fork");
            exit(1);
        }
        else if (childPIDS[i] == 0)
        {
            close(pipefd[0]);
            write(pipefd[1], msg[i], strlen(msg[i]) + 1);
            exit(0);
        }
    }

    for (size_t i = 0; i < 2; i++)
    {
        pid_t child_pid;
        int status;

        child_pid = wait(&status);
        if (child_pid == -1)
        {
            perror("Can't wait\n");
            exit(1);
        }
        if (WIFEXITED(status))
            printf("Child (PID = %d) exited with status = %d\n", child_pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Child (PID = %d) with (signal %d) killed\n", child_pid, WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Child (PID = %d) with (signal %d) stopped\n", child_pid, WSTOPSIG(status));
    }

    ssize_t s;

    close(pipefd[1]);

    for (size_t i = 0; i < 3; i++)
    {
        s = read(pipefd[0], &buf, strlen(msg[i]) + 1);
        printf("%ld, %s\n", s, buf);
        buf[0] = '\0';
    }

    close(pipefd[0]);

    return 0;
}
