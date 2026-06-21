#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "common.h"

#define MAX_EVENTS 16
#define EPOLL_RUN_TIMEOUT 500

#define SEM_EMPTY 0
#define SEM_FULL 1

int semid;
struct sembuf start_produce[] = {{SEM_EMPTY, -1, IPC_NOWAIT}};
struct sembuf stop_produce[] = {{SEM_FULL, 1, IPC_NOWAIT}};
struct sembuf start_consume[] = {{SEM_FULL, -1, IPC_NOWAIT}};
struct sembuf stop_consume[] = {{SEM_EMPTY, 1, IPC_NOWAIT}};

char ring[ARR_SIZE];
int head = 0;
int tail = 0;

FILE *logfile;

volatile int flag = 1;
void sig_handler(int sig_num) {
    printf("recieved signal %d\n", sig_num);
    flag = 0;
}

void serve(void *args)
{
    int connfd = *((int *)args);
    req_t req;
    ssize_t bytes;
    struct timespec t0, t1;
    long ns;

    free(args);

    while (1) {
        bytes = recv(connfd, &req, sizeof(req), 0);
        if (bytes == 0)
            break;
        else if (bytes == -1) {
            perror("recv");
            exit(1);
        }

        clock_gettime(CLOCK_MONOTONIC, &t0);

        switch (req.type)
        {
            case 'p':
                if (semop(semid, start_produce, 1) == -1) {
                    if (errno == EAGAIN) {
                        req.status = ERR_BUF_FULL;
                        printf("prod: buf_full\n");
                    } else {
                        perror("semop");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    ring[tail] = req.letter;
                    tail = (tail + 1) % ARR_SIZE;
                    req.status = OK;
                    if (semop(semid, stop_produce, 1) == -1) {
                        if (errno == EAGAIN) {
                            req.status = ERR_BUF_EMPTY;
                            printf("prod: buf_empty\n");
                        } else {
                            perror("semop");
                            exit(EXIT_FAILURE);
                        }
                    }
                    printf("prod: %c\n", req.letter);
                }
                break;
            case 'c':
                if (semop(semid, start_consume, 1) == -1) {
                    if (errno == EAGAIN) {
                        req.status = ERR_BUF_EMPTY;
                        printf("cons: buf_empty\n");
                    } else {
                        perror("semop");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    req.letter = ring[head];
                    head = (head + 1) % ARR_SIZE;
                    req.status = (char)OK;
                    if (semop(semid, stop_consume, 1) == -1) {
                        if (errno == EAGAIN) {
                            req.status = ERR_BUF_FULL;
                            printf("cons: buf_full\n");
                        } else {
                            perror("semop");
                            exit(EXIT_FAILURE);
                        }
                    }
                    printf("cons: %c\n", req.letter);
                }
                break;
        }

        send(connfd, &req, sizeof(req), 0);

        clock_gettime(CLOCK_MONOTONIC, &t1);
        ns = (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec);
        fprintf(logfile, "%c %ld\n", req.type, ns);
    }

    close(connfd);
}

int main(int argc, char **argv)
{
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    struct sigaction sa;
    int skfd, epfd, fd;
    int i;
    int opt = 1;
    int *p;
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds;

    if (argc != 2) {
        printf("Wrong arg count\n");
        exit(1);
    }

    logfile = fopen(argv[1], "w");
    if (!logfile)
    {
        perror("fopen");
        exit(1);
    }

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semid == -1)
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    semctl(semid, SEM_EMPTY, SETVAL, ARR_SIZE);
    semctl(semid, SEM_FULL, SETVAL, 0);

    skfd = socket(AF_INET, SOCK_STREAM, 0);
    if (fcntl(skfd, F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(1);
    }
    setsockopt(skfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    if (bind(skfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(skfd, 1024) == -1) {
        perror("listen");
        exit(1);
    }

    printf("port: %d\n", SERV_PORT);

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        exit(1);
    }
    ev.events = EPOLLIN;
    ev.data.fd = skfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, skfd, &ev) == -1) {
        perror("eporll_ctl");
        exit(1);
    }

    while (flag) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, EPOLL_RUN_TIMEOUT);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(1);
        }

        for (i = 0; i < nfds; ++i) {
            fd = events[i].data.fd;

            clilen = sizeof(cliaddr);
            p = malloc(sizeof(int));
            *p = accept(fd, (struct sockaddr *)&cliaddr, &clilen);
            if (*p == -1) {
                perror("accept");
                printf("errno = %d\n", errno);
            }
            serve(p);
        }
    }

    close(skfd);
    close(epfd);
    fclose(logfile);
    semctl(semid, 0, IPC_RMID);

    exit(0);
}
