cmake_minimum_required(VERSION 3.15)
project(task2 C)

set(CMAKE_C_STANDARD 11)
find_package (Threads)

add_executable(task2
        water.c)

target_link_libraries (task2 ${CMAKE_THREAD_LIBS_INIT})
