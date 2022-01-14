cmake_minimum_required (VERSION 3.22.0)
project (my-libuv)

set(sourceDir ${CMAKE_SOURCE_DIR}/src)

set(sourceFile ${sourceDir}/main.c
        ${sourceDir}/loop.c
        ${sourceDir}/core.c
        ${sourceDir}/timer.c
        ${sourceDir}/pthread.c
       )

add_executable(my-libuv ${sourceFile})
