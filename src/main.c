#include "stdio.h"
#include "heap-inl.h"
#include "stdlib.h"
#include "time.h"
#include "../include/uv.h"

extern int num;


int main() {
   uv_loop_t *loop = uv_loop_default();
   uv_timer_t handle;

   uv_timer_init(loop, &handle);
   uv_run(loop, UV_RUN_DEFAULT);
}