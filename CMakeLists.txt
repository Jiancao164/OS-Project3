cmake_minimum_required(VERSION 3.19)
project(Project3 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Project3
        fs5600.h
        homework.c
        hw3fuse.c
        misc.c
        unittest-1.c
        unittest-2.c test1.c)
