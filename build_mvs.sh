#!/bin/bash

gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/__memmgr.c -o common/src/__memmgr.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/assert.c -o common/src/assert.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/ctype.c -o common/src/ctype.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/errno.c -o common/src/errno.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/locale.c -o common/src/locale.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/math.c -o common/src/math.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/setjmp.c -o common/src/setjmp.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/signal.c -o common/src/signal.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/start.c -o common/src/start.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/stdio.c -o common/src/stdio.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/stdlib.c -o common/src/stdlib.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/string.c -o common/src/string.s
gccmvs -Os -S -ansi -pedantic-errors -DXXX_MEMMGR -I common/include/ common/src/time.c -o common/src/time.s

 m4 -I mvs/maclib/ -I mvs/asm/ -I common/src/ job_template.m4 > job.jcl
 nc -w1 <HOST> <PORT> < job.jcl
