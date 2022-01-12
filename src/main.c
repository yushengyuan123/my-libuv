#include "stdio.h"
#include "heap-inl.h"
#include "stdlib.h"
#include "../include/uv.h"

extern int num;


int main() {
   uv_loop_t *loop = uv_loop_default();

   uv_loop_init(loop);
}