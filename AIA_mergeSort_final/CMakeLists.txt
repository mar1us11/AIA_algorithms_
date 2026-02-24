cmake_minimum_required(VERSION 3.30)
project(AIA_mergeSort_final C)

set(CMAKE_C_STANDARD 23)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -funroll-loops")

add_executable(AIA_mergeSort_final main.c)
