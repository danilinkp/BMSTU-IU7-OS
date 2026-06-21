/*
 * client_epoll.c — клиент для server_select
 *
 * Сервер однопоточный и не может заблокироваться — при полном/пустом
 * буфере сразу возвращает BUFFER_FULL_ERR / BUFFER_EMPTY_ERR.
 * Клиент сам повторяет запрос (busy-wait с паузой) пока не получит OK.
 */
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

static char letter = 'a';

static int do_connect(const struct sockaddr_in *srv)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }
    if (connect(sockfd, (const struct sockaddr *)srv, sizeof(*srv)) == -1)
        { perror("connect"); exit(1); }
    return sockfd;
}

void produce(const struct sockaddr_in *srv)
{
    char buf[BUF_SIZE];
    int status;

    do {
        int sockfd = do_connect(srv);
        buf[0] = 'P';
        buf[1] = letter;
        send(sockfd, buf, sizeof(buf), 0);
        recv(sockfd, &status, sizeof(status), 0);
        close(sockfd);

        if (status == BUFFER_FULL_ERR)
        {
            printf("Producer: buffer full, retrying...\n");
            usleep(1000);
        }
    } while (status == BUFFER_FULL_ERR);

    printf("Producer sent: %c  Response: OK\n", buf[1]);
    letter = (letter == 'z') ? 'a' : letter + 1;
    usleep(50000);
}

void consume(const struct sockaddr_in *srv)
{
    char buf[BUF_SIZE];

    do {
        int sockfd = do_connect(srv);
        buf[0] = 'C';
        send(sockfd, buf, sizeof(buf), 0);
        recv(sockfd, buf, sizeof(buf), 0);
        close(sockfd);

        if ((unsigned char)buf[0] == BUFFER_EMPTY_ERR)
        {
            printf("Consumer: buffer empty, retrying...\n");
            usleep(100000);
        }
    } while ((unsigned char)buf[0] == BUFFER_EMPTY_ERR);

    printf("Consumer got: %c  Response: OK\n", buf[1]);
    usleep(50000);
}

int main(int argc, char **argv)
{
    struct sockaddr_in servaddr;

    if (argc != 3)
    {
        fprintf(stderr, "usage: client_epoll <IPaddress> <p|c>\n"
                        "  p - producers dominate (every 3rd is consumer)\n"
                        "  c - consumers dominate (every 3rd is producer)\n");
        exit(1);
    }

    int producer_mode = (argv[2][0] == 'p');

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1)
        { perror("inet_pton"); exit(1); }

    for (int iter = 0; iter < 300; iter++)
    {
        printf("[iter %d]\n", iter);

        if (producer_mode)
        {
            if (iter % 3 == 2) consume(&servaddr);
            else               produce(&servaddr);
        }
        else
        {
            if (iter % 3 == 2) produce(&servaddr);
            else               consume(&servaddr);
        }
    }

    return 0;
}