cmake_minimum_required (VERSION 3.22.0)
project (my-libuv)

set(sourceDir ${CMAKE_SOURCE_DIR}/src)

set(sourceFile ${sourceDir}/main.c
        ${sourceDir}/loop.c
        src/uv-common.h)

add_executable(my-libuv ${sourceFile})
