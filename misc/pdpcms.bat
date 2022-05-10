rem If pdptop is set to S/370, you don't need MEMMGR
rem if it is set to S/380, you do need MEMMGR, because
rem it does an ATL GETMAIN which only works once, with
rem current VM/380 technology

del pdpcms.zip
gcccms -Os -DXXX_MEMMGR -S -I . start.c
gcccms -Os -DXXX_MEMMGR -S -I . stdio.c
gcccms -Os -DXXX_MEMMGR -S -I . stdlib.c
gcccms -Os -DXXX_MEMMGR -S -I . ctype.c
gcccms -Os -DXXX_MEMMGR -S -I . string.c
gcccms -Os -DXXX_MEMMGR -S -I . time.c
gcccms -Os -DXXX_MEMMGR -S -I . errno.c
gcccms -Os -DXXX_MEMMGR -S -I . assert.c
gcccms -Os -DXXX_MEMMGR -S -I . locale.c
gcccms -Os -DXXX_MEMMGR -S -I . math.c
gcccms -Os -DXXX_MEMMGR -S -I . setjmp.c
gcccms -Os -DXXX_MEMMGR -S -I . signal.c
gcccms -Os -DXXX_MEMMGR -S -I . __memmgr.c
gcccms -Os -DXXX_MEMMGR -S -I . pdptest.c
zip -0X pdpcms *.s *.exec *.asm *.mac

rem Useful for VM/380
call runcms pdpcms.exec output.txt pdpcms.zip

rem Useful for z/VM
rem mvsendec encb pdpcms.zip pdpcms.dat
rem loc2ebc pdpcms.dat xfer.card 80
