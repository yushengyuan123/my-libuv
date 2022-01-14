#include "stdio.h"
#include "pthread.h"
#include "uv-common.h"

void uv_once(uv_once_t* guard, void (*callback)(void)) {
    pthread_once(guard, callback);
}

int uv_cond_init(uv_cond_t* cond) {
    return UV__ERR(pthread_cond_init(cond, NULL));
}