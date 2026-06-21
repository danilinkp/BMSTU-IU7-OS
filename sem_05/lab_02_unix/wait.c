#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(void)
{
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
            printf("Child: PID = %d, PPID = %d, group = %d\n", getpid(), getppid(), getpgrp());
            if (i == 0)
                sleep(2);
            exit(0);
        }
    }
    printf("Parent: PID = %d, group = %d, children = %d, %d\n", getpid(), getpgrp(), childPIDS[0], childPIDS[1]);

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

    return 0;
}
