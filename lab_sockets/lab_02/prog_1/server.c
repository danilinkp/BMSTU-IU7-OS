#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define SOCK_NAME "./server.soc"
#define BUFF_SIZE 1024

int flag = 1;
void handler(int s_num) {
    printf("signal recieved %d\n", s_num);
    flag = 0;
}

int main()
{
    int sock_fd;
    unsigned int addrlen;
    struct sockaddr addr, clnt_addr;
    char buff[BUFF_SIZE];
    int num_chars;

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    addr.sa_family = AF_UNIX;
    strncpy(addr.sa_data, SOCK_NAME, strlen(SOCK_NAME));
    if (bind(sock_fd, &addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }
    signal(SIGALRM, handler);
    alarm(10);
    while (flag) {
        num_chars = recvfrom(sock_fd, buff, BUFF_SIZE, 0, &clnt_addr, &addrlen);
        if (num_chars < 0) {
            perror("recvfrom");
            exit(1);
        }
        buff[num_chars] = '\0';
        printf("read: %s\n", buff);
    }
    unlink(SOCK_NAME);
    close(sock_fd);

    exit(0);
} 
