* $$ JOB JNM=VSEJOB
* $$ LST LST=SYSLST,CLASS=A
// JOB VSEJOB
*
*
*
* Standard assignments for C programs
*
// ASSGN SYS000,SYSLNK
// ASSGN SYS005,SYSLST
// ASSGN SYS007,SYSPCH
// OPTION DUMP
*
*
*
* Get the direct macros in
*
// OPTION EDECK,NODECK  
// DLBL IJSYSPH,'PDPPUNCH.DAT',0,SD
// EXTENT SYSPCH,,,,10000,100
ASSGN SYSPCH,SYS000
// EXEC ASSEMBLY
undivert(pdpprlg.mac)dnl
undivert(pdpepil.mac)dnl
         END
/*
CLOSE SYSPCH,PUNCH
// OPTION NOEDECK
*
*
*
*
// DLBL IJSYSIN,'PDPPUNCH.DAT'
ASSGN SYSIPT,SYS000
// EXEC MAINT                                         
CLOSE SYSIPT,READER
*
*
*
*
* Now do the copy libs
*
// EXEC MAINT
 CATALS A.PDPTOP
 BKEND
undivert(pdptop.mac)dnl
 BKEND
/*
*
*
* Now assemble the main routine
*
// OPTION CATAL
 PHASE PDPTEST,S+80
// EXEC ASSEMBLY
undivert(vsestart.asm)dnl
/*
*
* Now assemble the subroutines
*
// EXEC ASSEMBLY
undivert(start.s)dnl
/*
// EXEC ASSEMBLY
undivert(stdio.s)dnl
/*
// EXEC ASSEMBLY
undivert(stdlib.s)dnl
/*
// EXEC ASSEMBLY
undivert(ctype.s)dnl
/*
// EXEC ASSEMBLY
undivert(string.s)dnl
/*
// EXEC ASSEMBLY
undivert(time.s)dnl
/*
// EXEC ASSEMBLY
undivert(errno.s)dnl
/*
// EXEC ASSEMBLY
undivert(assert.s)dnl
/*
// EXEC ASSEMBLY
undivert(locale.s)dnl
/*
// EXEC ASSEMBLY
undivert(math.s)dnl
/*
// EXEC ASSEMBLY
undivert(setjmp.s)dnl
/*
// EXEC ASSEMBLY
undivert(signal.s)dnl
/*
// EXEC ASSEMBLY
undivert(__memmgr.s)dnl
/*
// EXEC ASSEMBLY
undivert(pdptest.s)dnl
/*
// EXEC ASSEMBLY
undivert(vsesupa.asm)dnl
/*
*
* Now link the whole app
*
// EXEC LNKEDT
*
*
*
* Now run the app
*
// EXEC PDPTEST,SIZE=AUTO,PARM='PaulEdwards two Three'
*
*
*
/&
* $$ EOJ
