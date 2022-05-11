cmake_minimum_required(VERSION 3.13)
project(mvsclib C)

SET(CMAKE_C_STANDARD 90)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__CROSS__")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fPIC -std=c89 -stdlib=libc++ -Wall")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__DEBUG__ -D__MVS__")

include_directories(inc)
include_directories(fss)

#include_directories(mvs)
#include_directories(dynit)
#include_directories(rac)
#include_directories(smf)


add_library(mvsclib
        inc/stddef.h
        inc/stdio.h
        src/stdio.c
        )


if(UNIX AND NOT APPLE)
    target_link_libraries(mvsclib m)
endif()