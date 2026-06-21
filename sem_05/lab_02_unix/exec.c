#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

int main(void)
{
    pid_t childPIDS[2];
    const char *exec_programs[2] = {"first_prog.out", "second_prog.out"};

    for (int i = 0; i < 2; i++)
    {
        if ((childPIDS[i] = fork()) == -1)
        {
            perror("Can't fork");
            exit(1);
        }
        else if (childPIDS[i] == 0)
        {
            printf("child: %d\n", getpid());
            if (execl(exec_programs[i], exec_programs[i], NULL) == -1)
            {
                perror("Can't exec");
                exit(1);
            }
        }
        else
            printf("parent: %d\n", getpid());
    }


    for (size_t i = 0; i < 2; i++)
    {
        int status;
        pid_t w = wait(&status);
        if (w == -1)
        {
            perror("Can't wait\n");
            exit(1);
        }
        if (WIFEXITED(status))
            printf("Child (PID = %d) exited with status = %d\n", w, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Child (PID = %d) with (signal %d) killed\n", w, WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Child (PID = %d) with (signal %d) stopped\n", w, WSTOPSIG(status));
    }

    return 0;
}
