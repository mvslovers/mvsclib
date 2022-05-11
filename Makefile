# This is the makefile for Linux to build pdpclib using
# the gcc compiler.
#

CC=gcc
COPTS=-c -O2 -fno-builtin -DUSE_MEMMGR -I common/include -m32 -fno-stack-protector

C_SOURCE:=$(wildcard \
	./common/src/*.c \
)

OBJ:=$(subst .c,.o,$(C_SOURCE))

pdplinux.a: $(OBJ) linux/asm/linsupa.o
	ar r linux/pdplinux.a $(OBJ) linux/asm/linsupa.o

.c.o:
	$(CC) $< $(COPTS) -o $@

linux/asm/linsupa.o: linux/asm/linsupa.asm
	as --32 -o $@ $<

clean:
	@rm -f common/src/*.o linux/asm/linsupa.o linux/pdplinux.a