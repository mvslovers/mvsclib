/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/PARM *
/INC PURGE
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPASM REPL
/inc rexx
parse arg name
queue "/file syspunch n("name".obj) new(repl) sp(50) secsp(100%)"
queue "/file syslib pds(@BLD000:*.M,$MCU:*.M,$MCM:*.M)"
queue "/load asm"
queue "/job nogo"
queue "/opt deck,list"
queue "/inc" name".asm"
"EXEC"

exit rc
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPTOP.M REPL
undivert(pdptop.mac)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPMAIN.M REPL
undivert(pdpmain.mac)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPPRLG.M REPL
undivert(pdpprlg.mac)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPEPIL.M REPL
undivert(pdpepil.mac)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE MUSSTART.ASM REPL
undivert(musstart.asm)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE MUSSUPA.ASM REPL
undivert(mussupa.asm)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE START.ASM REPL
undivert(start.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE STDIO.ASM REPL
undivert(stdio.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE STDLIB.ASM REPL
undivert(stdlib.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE CTYPE.ASM REPL
undivert(ctype.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE STRING.ASM REPL
undivert(string.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE TIME.ASM REPL
undivert(time.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE ERRNO.ASM REPL
undivert(errno.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE ASSERT.ASM REPL
undivert(assert.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE LOCALE.ASM REPL
undivert(locale.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE MATH.ASM REPL
undivert(math.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE SETJMP.ASM REPL
undivert(setjmp.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE SIGNAL.ASM REPL
undivert(signal.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE __MEMMGR.ASM REPL
undivert(__memmgr.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE PDPTEST.ASM REPL
undivert(pdptest.s)dnl
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/inc rexx
'pdpasm start'
'pdpasm stdio'
'pdpasm stdlib'
'pdpasm ctype'
'pdpasm string'
'pdpasm time'
'pdpasm errno'
'pdpasm assert'
'pdpasm locale'
'pdpasm math'
'pdpasm setjmp'
'pdpasm signal'
'pdpasm __memmgr'
'pdpasm mussupa'
'pdpasm musstart'
'pdpasm pdptest'
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/sys region=9999
/file lmod n(pdptest.lmod) new(repl) lr(128) recfm(f) sp(100) shr
/load lked
/job map,nogo,print,stats,mode=os,name=pdptest
.org 4a00
/inc pdptest.obj
/inc musstart.obj
/inc mussupa.obj
/inc __memmgr.obj
/inc assert.obj
/inc ctype.obj
/inc errno.obj
/inc locale.obj
/inc math.obj
/inc setjmp.obj
/inc signal.obj
/inc start.obj
/inc stdio.obj
/inc stdlib.obj
/inc string.obj
/inc time.obj
 ENTRY @@MAIN
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/sys region=3000,xregion=64m
/parm *
/inc dir
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/sys region=9999,xregion=64m
/file sysprint prt osrecfm(f) oslrecl(256)
/parm Hi there DeeeeeFerDog
/load xmon
pdptest n(pdptest.lmod) lcparm
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/SAVE RUNPDP REPL
/inc rexx
queue "/sys region=9999,xregion=64m"
queue "/file sysprint prt osrecfm(f) oslrecl(256)"
queue "/parm a1 b2"
queue "/load xmon"
queue "pdptest n(pdptest.lmod) lcparm"
"EXEC"

exit rc
/END

/ID SAVE-JOB-123456 @BLD000 9999 9999 9999 9999
/PASSWORD=BLD000
/inc rexx
'runpdp'
/END
