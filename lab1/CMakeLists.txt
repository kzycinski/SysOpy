cmake_minimum_required(VERSION 3.9)
project(lab1 C)

set(CMAKE_C_STANDARD 99)

add_library(lab1 library.c library.h test.c xd.c)

add_library(lab1 SHARED library.c library.h test.c xd.c)