# This is the makefile for Linux to build pdpclib using
# the gcc compiler.
#

CC=gcc
COPTS=-c -O2 -fno-builtin -DUSE_MEMMGR -I . -m32 -fno-stack-protector

pdplinux.a: linstart.o linsupa.o stdio.o string.o stdlib.o \
       start.o time.o errno.o assert.o signal.o locale.o \
       ctype.o setjmp.o math.o __memmgr.o
	ar r pdplinux.a linstart.o linsupa.o stdio.o string.o stdlib.o
	ar r pdplinux.a start.o time.o errno.o assert.o signal.o
	ar r pdplinux.a locale.o ctype.o setjmp.o math.o __memmgr.o
#	gcc -nostdlib -o pdptest linstart.o linsupa.o pdptest.o pdplinux.a

.c.o:
	$(CC) $(COPTS) $<

linsupa.o: linsupa.asm
	as --32 -o $@ $<

clean:
	@rm *.o pdplinux.a