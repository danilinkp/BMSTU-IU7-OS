#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define COUNT_ACTIVE_READERS 0
#define ACTIVE_WRITER 1
#define READ_COUNT 2
#define WRITE_COUNT 3

struct sembuf start_read[5] = {{READ_COUNT, 1, 0},
                               {ACTIVE_WRITER, 0, 0},
                               {WRITE_COUNT, 0, 0},
                               {COUNT_ACTIVE_READERS, 1, 0},
                               {READ_COUNT, -1, 0}};

struct sembuf stop_read[1] = {{COUNT_ACTIVE_READERS, -1, 0}};

struct sembuf start_write[5] = {{WRITE_COUNT, 1, 0},
                                {COUNT_ACTIVE_READERS, 0, 0},
                                {ACTIVE_WRITER, 0, 0},
                                {ACTIVE_WRITER, 1, 0},
                                {WRITE_COUNT, -1, 0}};

struct sembuf stop_write[1] = {{ACTIVE_WRITER, -1, 0}};

int flag = 1;
int count = 0;

void sig_handler(int sig_num)
{
    flag = 0;
    printf("%d catch signal: %d\n", getpid(), sig_num);
}

void reader(int semfd, char *addr)
{
    while (flag)
    {
        if (semop(semfd, start_read, 5) == -1)
        {
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
        printf("%d read %c\n", getpid(), *addr);
        if (semop(semfd, stop_read, 1) == -1)
        {
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
    }
}

void writer(int semfd, char *addr)
{
    while (flag)
    {
        if (semop(semfd, start_write, 5) == -1)
        {
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
        (*addr)++;
        if (*addr == 'z' + 1)
        {
            count++;
            *addr = 'a';
        }
        if (count == 2)
            kill(0, SIGTERM);
        printf("%d write %c\n", getpid(), *addr);

        if (semop(semfd, stop_write, 1) == -1)
        {
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
    }
}

int main(void)
{
    int semfd, shmfd;
    char *addr;
    pid_t childpid[7];
    semfd = semget(IPC_PRIVATE, 4, IPC_CREAT | 0666);
    if (semfd == -1)
    {
        perror("semget");
        return 1;
    }
    if (semctl(semfd, COUNT_ACTIVE_READERS, SETVAL, 0) == -1)
    {
        perror("semctl");
        return 1;
    }
    if (semctl(semfd, ACTIVE_WRITER, SETVAL, 0) == -1)
    {
        perror("semctl");
        return 1;
    }
    if (semctl(semfd, READ_COUNT, SETVAL, 0) == -1)
    {
        perror("semctl");
        return 1;
    }
    if (semctl(semfd, WRITE_COUNT, SETVAL, 0) == -1)
    {
        perror("semctl");
        return 1;
    }
    shmfd = shmget(IPC_PRIVATE, getpagesize(), IPC_CREAT | 0666);
    if (shmfd == -1)
    {
        perror("shmget");
        return 1;
    }
    addr = (char *)shmat(shmfd, 0, 0);
    if (addr == (char *)-1)
    {
        perror("shmat");
        return 1;
    }
    *addr = 'a' - 1;

    if (signal(SIGTERM, sig_handler) == SIG_ERR)
    {
        perror("signal");
        return 1;
    }
    for (int i = 0; i < 5; i++)
    {
        if ((childpid[i] = fork()) == -1)
        {
            perror("Can't fork");
            exit(1);
        }
        else if (childpid[i] == 0)
        {
            // alarm(1);
            writer(semfd, addr);
            exit(0);
        }
    }
    for (int i = 5; i < 7; i++)
    {
        if ((childpid[i] = fork()) == -1)
        {
            perror("Can't fork");
            exit(1);
        }
        else if (childpid[i] == 0)
        {
            // alarm(1);
            reader(semfd, addr);
            exit(0);
        }
    }

    for (size_t i = 0; i < 7; i++)
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
            printf("%d exited with status = %d\n", child_pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("%d with (signal %d) killed\n", child_pid, WTERMSIG(status));
        else if (WIFSTOPPED(status))
            printf("%d with (signal %d) stopped\n", child_pid, WSTOPSIG(status));
    }

    if (shmdt(addr) == -1)
    {
        perror("shmdt");
        return 1;
    }
    if (semctl(semfd, 0, IPC_RMID) == -1)
    {
        perror("semctl");
        return 1;
    }
    if (shmctl(shmfd, IPC_RMID, 0) == -1)
    {
        perror("shmctl");
        return 1;
    }

    return 0;
}