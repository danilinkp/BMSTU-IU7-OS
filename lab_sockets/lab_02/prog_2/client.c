#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SOCK_NAME "./server.soc"
#define BUF_SIZE 1024

int main()
{
    int fd;
    struct sockaddr server_addr, client_addr;
    socklen_t addrlen;
    char buf[BUF_SIZE];
    char client_path[256];
    ssize_t num_chars;
    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("socket");
        exit(1);
    }
    sprintf(client_path, "./%d_client.soc", getpid());

    server_addr.sa_family = AF_UNIX;
    strncpy(server_addr.sa_data, SOCK_NAME, strlen(SOCK_NAME));
    client_addr.sa_family = AF_UNIX;
    strncpy(client_addr.sa_data, client_path, strlen(client_path));
    if (bind(fd, &client_addr, sizeof(client_addr)) < 0) {
        perror("bind");
        close(fd);
        exit(1);
    }
    sprintf(buf, "%d", getpid());
    if (sendto(fd, buf, strlen(buf), 0, &server_addr, sizeof(server_addr)) == -1) {
        perror("sendto");
        unlink(client_path);
        close(fd);
        exit(1);
    }
    printf("send: %s\n", buf);
    num_chars = recvfrom(fd, buf, BUF_SIZE, 0, &client_addr, &addrlen);
    if (num_chars == -1) {
        perror("recvfrom");
        unlink(client_path);
        close(fd);
        exit(1);
    }
    buf[num_chars] = '\0';
    printf("read: %s\n", buf);
    sleep(10);
    unlink(client_path);
    close(fd);
    exit(0);
}
