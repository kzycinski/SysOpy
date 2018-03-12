cmake_minimum_required(VERSION 3.9)
project(sysopy C)

set(CMAKE_C_STANDARD 99)

add_library(sysopy library.c library.h main.c)