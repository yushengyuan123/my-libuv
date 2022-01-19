#include "stdio.h"
#include "heap-inl.h"
#include "stdlib.h"
#include "time.h"
#include "../include/uv.h"
#include "internal.h"
#include "sys/stat.h"

int repeat = 0;
int repeatCount = 3;

void callback() {
    printf("文件操作回调函数\n");
}

struct Person {
    int a;
    void *wq[2];
};


void test(int *arr) {

}
//
//typedef struct uv_semaphore_s {
//    uv_mutex_t mutex;
//    uv_cond_t cond;
//    unsigned int value;
//} uv_semaphore_t;

int main() {
   uv_loop_t *loop = uv_loop_default();
   uv_timer_t handle;
   uv_fs_t fs;
   int res;

   res = uv_fs_mkdir(loop, &fs, "./nihao",
                     S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH, callback);

   uv_run(loop, UV_RUN_DEFAULT);
   printf("代码结束%d\n", res);
}