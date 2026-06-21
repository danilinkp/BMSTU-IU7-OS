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

#define N 26

#define BUFFER_EMPTY 0
#define BUFFER_FULL 1
#define BIN_SEM 2

struct sembuf start_produce[2] = {{BUFFER_EMPTY, -1, 0}, {BIN_SEM, -1, 0}};
struct sembuf stop_produce[2] = {{BIN_SEM, 1, 0}, {BUFFER_FULL, 1, 0}};

struct sembuf start_consume[2] = {{BUFFER_FULL, -1, 0}, {BIN_SEM, -1, 0}};
struct sembuf stop_consume[2] = {{BIN_SEM, 1, 0}, {BUFFER_EMPTY, 1, 0}};

int flag = 1;

void sig_handler(int sig_num)
{
    flag = 0;
    printf("%d catch signal: %d\n", getpid(), sig_num);
}

void producer(int semfd, char **pr_ptr, char *letter)
{
    while (flag)
    {
        if (semop(semfd, start_produce, 2) == -1)
        {
            perror("semop");
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
        char symb = (*letter)++;
        if (*letter == 'z' + 1)
            *letter = 'a';
        **pr_ptr = symb;
        (*pr_ptr)++;
        printf("Producer %d put: %c\n", getpid(), symb);
        if (semop(semfd, stop_produce, 2) == -1)
        {
            perror("semop");
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
    }
}

void consumer(int semfd, char **con_ptr)
{
    while (flag)
    {
        if (semop(semfd, start_consume, 2) == -1)
        {
            perror("semop");
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
        printf("Consumer %d get: %c\n", getpid(), **con_ptr);
        (*con_ptr)++;
        if (semop(semfd, stop_consume, 2) == -1)
        {
            perror("semop");
            printf("semop %d, errno %d\n", getpid(), errno);
            exit(1);
        }
    }
}

int main()
{
    int semfd = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (semfd == -1)
    {
        perror("semget");
        return 1;
    }
    if (semctl(semfd, BUFFER_EMPTY, SETVAL, N) == -1)
    {
        perror("semctl");
        return 1;
    }
    if (semctl(semfd, BUFFER_FULL, SETVAL, 0) == -1)
    {
        perror("semctl");
        return 1;
    }
    if (semctl(semfd, BIN_SEM, SETVAL, 1) == -1)
    {
        perror("semctl");
        return 1;
    }

    if (signal(SIGALRM, sig_handler) == SIG_ERR)
    {
        perror("signal");
        return 1;
    }

    int shmfd = shmget(IPC_PRIVATE, getpagesize(), IPC_CREAT | 0666);
    if (shmfd == -1)
    {
        perror("shmget");
        return 1;
    }
    char *base_addr = (char *)shmat(shmfd, 0, 0);
    if (base_addr == (char *)-1)
    {
        perror("shmat");
        return 1;
    }
    char **pr_ptr = (char **)base_addr;
    char **con_ptr = (char **)(base_addr + sizeof(char *));
    char *letter = base_addr + 2 * sizeof(char *);
    char *start_addr = letter + sizeof(char);
    *pr_ptr = start_addr;
    *con_ptr = start_addr;
    *letter = 'a';

    pid_t childpid[8];
    for (int i = 1; i <= 8; i++)
    {
        if ((childpid[i] = fork()) == -1)
        {
            perror("Can't fork");
            exit(1);
        }
        else if (childpid[i] == 0)
        {
            alarm(1);
            if (i % 3 == 0)
                producer(semfd, pr_ptr, letter);
            else
                consumer(semfd, con_ptr);
            exit(0);
        }
    }

    for (size_t i = 0; i < 8; i++)
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

    if (shmdt(base_addr) == -1)
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