#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

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
            sleep(2);
            printf("Child: PID = %d, PPID = %d, group = %d\n", getpid(), getppid(), getpgrp());
            exit(0);
        }
    }
    printf("Parent: PID = %d, group = %d, children = %d, %d\n", getpid(), getpgrp(), childPIDS[0], childPIDS[1]);

    return 0;
}
