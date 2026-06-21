#ifndef COMMON_H
#define COMMON_H

#define SERV_PORT  9877
#define BUF_SIZE   64
#define ARR_SIZE   26

#define OK 0
#define ERR_BUF_FULL 1
#define ERR_BUF_EMPTY 2

typedef struct {
    char type;
    int status;
    char letter;
} req_t;

#endif
