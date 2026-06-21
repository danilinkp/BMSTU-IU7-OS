#include <math.h>
#include <time.h>
#include <unistd.h>

#include "bakery.h"

static int next_number = 1;
static int current_number = 1;
static pthread_mutex_t number_mutex = PTHREAD_MUTEX_INITIALIZER;

void* find_max_number(void* ptr) {
    struct svc_req* rqstp = (struct svc_req*)ptr;
    static int result;
    pthread_mutex_lock(&number_mutex);
    next_number++;
    result = next_number;
    if (!svc_sendreply(rqstp, (xdrproc_t)xdr_int, (char*)&result)) {
        svcerr_systemerr(rqstp);
    }
    pthread_mutex_unlock(&number_mutex);
    free(rqstp);
    return NULL;
}

bool
get_number_1_svc(struct svc_req* rqstp) {
    pthread_t tid;
    struct svc_req* arg = malloc(sizeof(struct svc_req));
    if (!arg)
        return FALSE;
    *arg = *rqstp;
    if (pthread_create(&tid, NULL, find_max_number, arg) == -1) {
        free(arg);
        return FALSE;
    }
    pthread_detach(tid);
    return TRUE;
}

int bakery_service_1_svc(int num, struct svc_req* rqstp) {
    static int result;
    if (num != current_number) {
        result = -1;
        return result;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    result = (ts.tv_sec * 1000000000 + ts.tv_nsec) / 1000000;
    current_number++;
    return result;
}
