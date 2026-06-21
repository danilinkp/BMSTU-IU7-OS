#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/sem.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "common.h"

int semid;
int listenfd;

char ring[ARR_SIZE];
int head = 0;
int tail = 0;

#define SEM_EMPTY 0
#define SEM_FULL 1
#define SEM_MUTEX 2

struct sembuf start_produce[] = {{SEM_EMPTY, -1, 0}, {SEM_MUTEX, -1, 0}};
struct sembuf stop_produce[] = {{SEM_MUTEX, 1, 0}, {SEM_FULL, 1, 0}};
struct sembuf start_consume[] = {{SEM_FULL, -1, 0}, {SEM_MUTEX, -1, 0}};
struct sembuf stop_consume[] = {{SEM_MUTEX, 1, 0}, {SEM_EMPTY, 1, 0}};
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int flag = 1;
static FILE *logfile;

void sig_handler(int sig_num) {
    printf("recieved signal %d\n", sig_num);
    flag = 0;
}

static void log_time(const char *op, long ns)
{
    pthread_mutex_lock(&log_mutex);
    fprintf(logfile, "%s %ld\n", op, ns);
    pthread_mutex_unlock(&log_mutex);
}

void *process_client(void *args)
{
    int connfd = *((int *)args);
    free(args);
    int status;
    req_t req;
    struct timespec t0, t1;

    while (1)
    {
        if (recv(connfd, &req, sizeof(req), 0) <= 0)
            break;

        clock_gettime(CLOCK_MONOTONIC, &t0);

        switch (req.type)
        {
        case 'p':
            if (semop(semid, start_produce, 2) == -1)
            {
                perror("semop");
                exit(EXIT_FAILURE);
            }

            ring[tail] = req.letter;
            tail = (tail + 1) % ARR_SIZE;
            status = OK;
            printf("producer put: %c\n", req.letter);

            if (semop(semid, stop_produce, 2) == -1)
            {
                perror("semop");
                exit(EXIT_FAILURE);
            }

            clock_gettime(CLOCK_MONOTONIC, &t1);
            send(connfd, &status, sizeof(status), 0);
            break;

        case 'c':
            if (semop(semid, start_consume, 2) == -1)
            {
                perror("semop");
                exit(EXIT_FAILURE);
            }

            req.letter = ring[head];
            head = (head + 1) % ARR_SIZE;
            printf("consumer got: %c\n", req.letter);

            if (semop(semid, stop_consume, 2) == -1)
            {
                perror("semop");
                exit(EXIT_FAILURE);
            }

            clock_gettime(CLOCK_MONOTONIC, &t1);
            req.status = (char)OK;
            send(connfd, &req, sizeof(req), 0);
            break;

        default:
            clock_gettime(CLOCK_MONOTONIC, &t1);
            break;
        }

        long ns = (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec);
        log_time(req.type == 'p' ? "p" : "c", ns);
    }

    close(connfd);
    return NULL;
}

int main(int argc, char **argv)
{
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    pthread_t th;
    struct sigaction sa;

    if (argc != 2) {
        printf("Wrong arg count\n");
        exit(1);
    }

    logfile = fopen(argv[1], "w");
    if (!logfile)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    semctl(semid, SEM_EMPTY, SETVAL, ARR_SIZE);
    semctl(semid, SEM_FULL, SETVAL, 0);
    semctl(semid, SEM_MUTEX, SETVAL, 1);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 1024) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("listening on port %d\n", SERV_PORT);

    while (flag)
    {
        clilen = sizeof(cliaddr);
        int *p = malloc(sizeof(int));
        *p = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (*p < 0) {
            perror("accept");
            printf("errno = %d\n", errno);
        }
        pthread_create(&th, NULL, process_client, p);
        pthread_detach(th);
    }

    sleep(30);

    close(listenfd);
    semctl(semid, 0, IPC_RMID);
    fclose(logfile);

    exit(0);
}
