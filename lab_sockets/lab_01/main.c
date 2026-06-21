#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUF_SIZE 1024

char messages[3][10] = {"aaaaa", "bbb", "cccc"};

int main(int argc, char ** argv)
{
    int sockets[2];
    char buf[BUF_SIZE];
    int pid;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0)
    {
        perror("socketpair() failed");
        return EXIT_FAILURE;
    }


    for (int i = 0; i < 3; i++)
    {
        if ((pid = fork()) == -1)
        {
            perror("fork()");
            return EXIT_FAILURE;
        }
        if (pid == 0)
        {
            printf("child %d send: %s\n", getpid(), messages[i]);
            write(sockets[0], messages[i], strlen(messages[i]) + 1);
            read(sockets[0], buf, 3);
            printf("child %d receive: %s\n", getpid(), buf);
            exit(0);
        }
        else
        {
            read(sockets[1], buf, BUF_SIZE);
            printf("parent receive: %s\n", buf);
            strcat(buf, " OK");
            printf("parent send: %s\n", buf);
            write(sockets[1], buf, sizeof(buf));
        }
    }

    for (int i = 0; i < 3; i++)
    {
        int status;
        int pid_child = wait(&status);
        if (WIFEXITED(status))
            printf("Child %d exited with status %d\n", pid_child, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("Child %d was terminated by signal %d\n", pid_child, WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("Child %d was stopped by signal %d\n", pid_child, WSTOPSIG(status));
        else
            printf("Unexpected child status\n");
    }

    exit(0);
}