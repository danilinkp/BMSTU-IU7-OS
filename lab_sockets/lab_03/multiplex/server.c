/*
 * server_epoll.c — однопоточная версия с мультиплексированием (epoll)
 *
 * Синхронизация: только два считающих семафора SEM_EMPTY + SEM_FULL.
 * SEM_MUTEX не нужен — однопоточный сервер не имеет гонок на head/tail.
 *
 * Семафоры с IPC_NOWAIT: сервер не может заблокироваться — иначе
 * он перестанет обслуживать всех остальных клиентов.
 * При BUFFER_FULL/EMPTY возвращает клиенту код ошибки,
 * клиент повторяет запрос сам.
 *
 * Время обслуживания каждого запроса пишется в timing_epoll.log.
 */

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/sem.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "common.h"

#define MAX_EVENTS 64

int semid;
int listenfd;
int epollfd;

char ring[ARR_SIZE];
int  head = 0;
int  tail = 0;

#define SEM_EMPTY  0
#define SEM_FULL   1

struct sembuf start_produce[] = {{SEM_EMPTY, -1, IPC_NOWAIT}};
struct sembuf stop_produce[]  = {{SEM_FULL,   1, 0}};

struct sembuf start_consume[] = {{SEM_FULL,  -1, IPC_NOWAIT}};
struct sembuf stop_consume[]  = {{SEM_EMPTY,  1, 0}};

static FILE *logfile;

void handle_client_command(int fd, char *buf)
{
    int status;
    char item;
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    switch (buf[0])
    {
        case 'P':
            item = buf[1];
            if (semop(semid, start_produce, 1) == -1)
            {
                if (errno == EAGAIN)
                {
                    status = BUFFER_FULL_ERR;
                    printf("[epoll] Producer: buffer full\n");
                    clock_gettime(CLOCK_MONOTONIC, &t1);
                    send(fd, &status, sizeof(status), 0);
                    break;
                }
                perror("semop"); exit(EXIT_FAILURE);
            }
            ring[tail] = item;
            tail = (tail + 1) % ARR_SIZE;
            status = OK;
            printf("[epoll] Producer put: %c\n", item);
            semop(semid, stop_produce, 1);

            clock_gettime(CLOCK_MONOTONIC, &t1);
            send(fd, &status, sizeof(status), 0);
            break;

        case 'C':
            if (semop(semid, start_consume, 1) == -1)
            {
                if (errno == EAGAIN)
                {
                    status = BUFFER_EMPTY_ERR;
                    printf("[epoll] Consumer: buffer empty\n");
                    clock_gettime(CLOCK_MONOTONIC, &t1);
                    buf[0] = (char)status;
                    buf[1] = 0;
                    send(fd, buf, sizeof(buf), 0);
                    break;
                }
                perror("semop"); exit(EXIT_FAILURE);
            }
            item = ring[head];
            head = (head + 1) % ARR_SIZE;
            printf("[epoll] Consumer got: %c\n", item);
            semop(semid, stop_consume, 1);

            clock_gettime(CLOCK_MONOTONIC, &t1);
            buf[0] = (char)OK;
            buf[1] = item;
            send(fd, buf, sizeof(buf), 0);
            break;

        default:
            clock_gettime(CLOCK_MONOTONIC, &t1);
            break;
    }

    long ns = (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec);
    /* Запись в файл — после замера, не входит во время обслуживания */
    fprintf(logfile, "%c %ld\n", buf[0], ns);
}

void sigint_handler(int sig)
{
    (void)sig;
    close(listenfd);
    close(epollfd);
    semctl(semid, 0, IPC_RMID);
    if (logfile) fclose(logfile);
    exit(EXIT_SUCCESS);
}

int main(void)
{
    int connfd;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    struct epoll_event ev, events[MAX_EVENTS];
    char buf[BUF_SIZE];

    logfile = fopen("timing_epoll.log", "w");
    if (!logfile) { perror("fopen"); exit(EXIT_FAILURE); }

    signal(SIGINT, sigint_handler);

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    if (semid == -1) { perror("semget"); exit(EXIT_FAILURE); }
    semctl(semid, SEM_EMPTY, SETVAL, ARR_SIZE);
    semctl(semid, SEM_FULL,  SETVAL, 0);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(SERV_PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        { perror("bind"); exit(EXIT_FAILURE); }
    if (listen(listenfd, 1024) == -1)
        { perror("listen"); exit(EXIT_FAILURE); }

    epollfd = epoll_create1(0);
    if (epollfd == -1) { perror("epoll_create1"); exit(EXIT_FAILURE); }

    ev.events  = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
        { perror("epoll_ctl"); exit(EXIT_FAILURE); }

    printf("[epoll] listening on port %d\n", SERV_PORT);

    while (1)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) { perror("epoll_wait"); exit(EXIT_FAILURE); }

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            if (fd == listenfd)
            {
                clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                if (connfd == -1) { perror("accept"); continue; }

                ev.events  = EPOLLIN;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                    { perror("epoll_ctl"); exit(EXIT_FAILURE); }

                printf("[epoll] New connection fd=%d\n", connfd);
            }
            else
            {
                int n = recv(fd, buf, sizeof(buf), 0);
                if (n <= 0)
                {
                    printf("[epoll] fd=%d disconnected\n", fd);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                }
                else
                {
                    handle_client_command(fd, buf);
                }
            }
        }
    }
}