#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 64

const char *messages[6] = {
    "aaa",
    "bbbb",
    "cccccc"
};

int main()
{
    int sockets[2];
    pid_t pids[3];
    char buf[BUF_SIZE];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    {
        perror("socketpair");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 3; i++)
    {
        pids[i] = fork();

        if (pids[i] == -1)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pids[i] == 0)
        {
            close(sockets[1]);
            printf("child sent %s\n", messages[i]);
            write(sockets[0], messages[i], sizeof(messages[i]));
            read(sockets[0], buf, BUF_SIZE);
            printf("%s\n", buf);
            close(sockets[0]);
            exit(0);
        }
    }

    close(sockets[0]);

    for (int i = 0; i < 3; i++)
    {
        read(sockets[1], buf, BUF_SIZE);
        printf("parent received %s\n", buf);
        strcat(buf, " OK");

        write(sockets[1], buf, sizeof(buf));
    }

    close(sockets[1]);

    for (int i = 0; i < 3; i++)
    {
        int status;
        wait(&status);
        if (WIFEXITED(status))
        {
            printf("Child %d exited with code = %d\n", pids[i], WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child %d with (signal %d) killed\n", pids[i], WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            printf("Child %d with (signal %d) stopped\n", pids[i], WSTOPSIG(status));
        }
        else
        {
            printf("Unexpected status for Child\n");
        }
    }

    return 0;
}