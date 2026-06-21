#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "bakery.h"

int flag = 1;

void sig_handler(int sig_num) {
    flag = 0;
    printf("PID: %d\n", getpid());
}

void bakery_prog_1(char* host) {
    CLIENT* clnt;
    enum clnt_stat retval_1;
    int result_1;
    char* get_number_1_arg;
    enum clnt_stat retval_2;
    int result_2;

#ifndef DEBUG
    clnt = clnt_create(host, BAKERY_PROG, BAKERY_VER, "tcp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }
#endif

    result_1 = get_number_1(clnt);
    
    int received = 0;
    while (received == 0) {
        result_2 = bakery_service_1(result_1, clnt);
        
        if (result_2 > 0) {
            printf("%d: %ld\n", bakery_proc_1_arg, result_2);
            received = 1;
        } else {
            printf("Client %d waiting\n", bakery_proc_1_arg);
        }
    }
#ifndef DEBUG
    clnt_destroy(clnt);
#endif
}

int main(int argc, char* argv[]) {
    char* host;

    if (argc < 2) {
        printf("usage: %s server_host\n", argv[0]);
        exit(1);
    }
    host = argv[1];
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("signal");
        printf("errno: %d\n", errno);
        exit(1);
    }
    srand(getpid());
    while (flag) {
        bakery_prog_1(host);
    }
    exit(0);
}
