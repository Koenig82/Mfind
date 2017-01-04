cmake_minimum_required(VERSION 3.6)
project(mfind)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pthread -D_GNU_SOURCE")

set(SOURCE_FILES mfind.c mfind.h list_2cell.c list_2cell.h queue.c queue.h)
add_executable(mfind ${SOURCE_FILES})