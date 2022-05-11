cmake_minimum_required(VERSION 3.13)
project(mvsclib C)

enable_language(C ASM)

SET(CMAKE_C_STANDARD 90)
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__CROSS__")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
#SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D__DEBUG__ -D__MVS__")
SET(CMAKE_C_FLAGS "-c -O2 -fno-builtin -DUSE_MEMMGR -I common/include -m32 -fno-stack-protector")

SET(CMAKE_ASM_COMPILER as)
SET(CMAKE_ASM_FLAGS "--32")

add_library(mvsclib
    common/src/__memmgr.c  
    common/src/assert.c  
    common/src/ctype.c   
    common/src/errno.c   
    common/src/liballoc.c  
    common/src/linstart.c  
    common/src/locale.c    
    common/src/math.c      
    common/src/setjmp.c    
    common/src/signal.c  
    common/src/start.c   
    common/src/stdio.c   
    common/src/stdlib.c
    common/src/string.c
    common/src/time.c
    linux/asm/linsupa.asm
)

target_include_directories(mvsclib PRIVATE
    common/include
)

if(UNIX AND NOT APPLE)
    target_link_libraries(mvsclib m)
endif()