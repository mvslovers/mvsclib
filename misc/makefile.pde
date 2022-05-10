# For the 32-bit version of PDOS we define __PDOS386__
# Note that we link twice, because the second link
# doesn't report unresolved externals for some reason

CC=i686-elf-gcc
COPTS=-S -Os -fno-common -I. -I../src -D__32BIT__ -D__PDOS386__ -fleading-underscore

pdptest.exe: pdosst32.o pdptest.o stdio.o string.o stdlib.o \
       start.o time.o errno.o assert.o signal.o locale.o \
       ctype.o setjmp.o math.o pos.o support.o pdossupc.o bos.o \
       liballoc.o
  rm -f pdos.a
  i686-elf-ar r pdos.a pdosst32.o stdio.o string.o stdlib.o
  i686-elf-ar r pdos.a start.o time.o errno.o assert.o signal.o
  i686-elf-ar r pdos.a locale.o ctype.o setjmp.o math.o
  i686-elf-ar r pdos.a pos.o support.o pdossupc.o bos.o liballoc.o
  i686-elf-ld -s -e ___pdosst32 -o pdptest.exe pdosst32.o pdptest.o pdos.a
  i686-elf-ld -r -s -e ___pdosst32 -o pdptest.exe pdosst32.o pdptest.o pdos.a
  i686-elf-strip --strip-unneeded pdptest.exe

bos.o: ../src/bos.c
  $(CC) $(COPTS) -o $&.s ../src/bos.c
  i686-elf-as -o $@ $&.s
  rm -f $&.s

pos.o: ../src/pos.c
  $(CC) $(COPTS) -o $&.s ../src/pos.c
  i686-elf-as -o $@ $&.s
  rm -f $&.s

support.o: ../src/support.s
  i686-elf-as -o $@ ../src/support.s

.c.o:
  $(CC) $(COPTS) -o $&.s $<
  i686-elf-as -o $@ $&.s
  rm -f $&.s

.s.o:
  i686-elf-as -o $@ $<
