cmake_minimum_required (VERSION 3.22.0)
project (my-libuv)

set(sourceDir ${CMAKE_SOURCE_DIR}/src)

add_executable(my-libuv ${sourceDir}/main.c)
