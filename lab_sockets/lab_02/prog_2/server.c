#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define SOCK_NAME "./server.soc"
#define BUF_SIZE 1024

volatile int flag = 1;
void sig_handler(int sig_num) {
    printf("received signal %d\n", sig_num);
    flag = 0;
}

int main()
{
    int fd;
    struct sockaddr addr, clnt_addr;
    socklen_t addrlen;
    char buf[BUF_SIZE];
    int num_chars;
    struct timespec time;
    struct sigaction sa;

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }

    addr.sa_family = AF_UNIX;
    strncpy(addr.sa_data, SOCK_NAME, strlen(SOCK_NAME));

    if (bind(fd, &addr, sizeof(addr)) == -1) {
        perror("bind");
        close(fd);
        exit(1);
    }

    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    while (flag) {
        addrlen = sizeof(clnt_addr);
        num_chars = recvfrom(fd, buf, BUF_SIZE, 0, &clnt_addr, &addrlen);
        if (num_chars == -1) {
            perror("recvfrom");
            unlink(SOCK_NAME);
            close(fd);
            exit(1);
        }
        buf[num_chars] = '\0';
        printf("read: %s\n", buf);

        clock_gettime(CLOCK_MONOTONIC, &time);
        sprintf(buf, "%d\0", time.tv_nsec);

        if (sendto(fd, buf, strlen(buf), 0, &clnt_addr, sizeof(clnt_addr)) == -1) {
            perror("sendto");
            unlink(SOCK_NAME);
            close(fd);
            exit(1);
        }
        printf("send: %s\n", buf);
    }
    unlink(SOCK_NAME);
    close(fd);
    exit(0);
}
