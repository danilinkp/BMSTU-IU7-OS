#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

char letter = 'a';

static int do_connect(const struct sockaddr_in *srv)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        exit(1);
    }
    if (connect(sockfd, (const struct sockaddr *)srv, sizeof(*srv)) == -1)
    {
        perror("connect");
        exit(1);
    }
    return sockfd;
}

void produce(const struct sockaddr_in *srv)
{
    req_t req;
    int sockfd = do_connect(srv);

    req.type = 'p';
    req.letter = letter;
    send(sockfd, &req, sizeof(req), 0);

    recv(sockfd, &req, sizeof(req), 0);
    close(sockfd);

    printf("prod: ");
    switch (req.status) {
        case OK:
            printf("(ok)\n");
            break;
        case ERR_BUF_FULL:
            printf("(buf_full)\n");
            break;
        case ERR_BUF_EMPTY:
            printf("(buf_empty)\n");
            break;
    }

    letter = (letter == 'z') ? 'a' : letter + 1;

    usleep(20000);
}

void consume(const struct sockaddr_in *srv)
{
    req_t req;
    int sockfd = do_connect(srv);

    req.type = 'c';
    send(sockfd, &req, sizeof(req), 0);

    recv(sockfd, &req, sizeof(req), 0);
    close(sockfd);

    printf("cons: ");
    switch (req.status) {
        case OK:
            printf("%c\n", req.letter);
            break;
        case ERR_BUF_FULL:
            printf("(buf_full)\n");
            break;
        case ERR_BUF_EMPTY:
            printf("(buf_empty)\n");
            break;
    }

    usleep(20000);
}

int main(int argc, char **argv)
{
    struct sockaddr_in servaddr;

    if (argc != 3) {
        printf("usage: %s <port> <mode>\n", argv[0]);
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) == -1)
    {
        perror("inet_pton");
        exit(1);
    }
    for (int i = 0; i < 5; i++)
    {
        produce(&servaddr);
    }
    for (int iter = 0; iter < 40000; iter++) {
        // if (argv[2][0] == 'p')
        //     if (iter % 3 == 2)
        //         consume(&servaddr);
        //     else
        //         produce(&servaddr);
        // else
        if (iter % 2 == 0)
            produce(&servaddr);
        else
            consume(&servaddr);
    }

    return 0;
}
