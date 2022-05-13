//PDPMVS   JOB CLASS=C,MSGCLASS=X,REGION=0K
//*
//PDPASM   PROC LIST='NOLIST'
//ASM      EXEC PGM=IFOX00,
//   PARM='DECK,&LIST,TERM'
//SYSLIB   DD DSN=SYS1.MACLIB,DISP=SHR,DCB=BLKSIZE=32720
//         DD DSN=&&MACLIB,DISP=(OLD,PASS)
//         DD DSN=SYS1.AMODGEN,DISP=SHR
//SYSUT1   DD UNIT=SYSALLDA,SPACE=(CYL,(20,10))
//SYSUT2   DD UNIT=SYSALLDA,SPACE=(CYL,(20,10))
//SYSUT3   DD UNIT=SYSALLDA,SPACE=(CYL,(20,10))
//SYSTERM  DD SYSOUT=*
//SYSPRINT DD SYSOUT=*
//SYSLIN   DD DUMMY
//SYSGO    DD DUMMY
//SYSPUNCH DD DSN=&&OBJSET,UNIT=SYSALLDA,SPACE=(80,(9000,9000)),
//            DISP=(MOD,PASS)
//*
//         PEND
//CREATE   EXEC PGM=IEFBR14
//DD13     DD DSN=&&LOADLIB,DISP=(,PASS),
// DCB=(RECFM=U,LRECL=0,BLKSIZE=6144),
// SPACE=(CYL,(10,10,20)),UNIT=SYSALLDA
//DD14     DD DSN=&&MACLIB,DISP=(,PASS),
// DCB=(RECFM=FB,LRECL=80,BLKSIZE=6160),
// SPACE=(CYL,(10,10,20)),UNIT=SYSALLDA
//*
//PDPTOP   EXEC PGM=IEBGENER
//SYSUT2   DD  DSN=&&MACLIB(PDPTOP),DISP=(OLD,PASS)
//SYSUT1   DD  *
undivert(pdptop.mac)dnl
/*
//SYSPRINT DD  SYSOUT=*
//SYSIN    DD  DUMMY
//*
//PDPMAIN  EXEC PGM=IEBGENER
//SYSUT2   DD  DSN=&&MACLIB(PDPMAIN),DISP=(OLD,PASS)
//SYSUT1   DD  *
undivert(pdpmain.mac)dnl
/*
//SYSPRINT DD  SYSOUT=*
//SYSIN    DD  DUMMY
//*
//PDPPRLG  EXEC PGM=IEBGENER
//SYSUT2   DD  DSN=&&MACLIB(PDPPRLG),DISP=(OLD,PASS)
//SYSUT1   DD  *
undivert(pdpprlg.mac)dnl
/*
//SYSPRINT DD  SYSOUT=*
//SYSIN    DD  DUMMY
//*
//PDPEPIL  EXEC PGM=IEBGENER
//SYSUT2   DD  DSN=&&MACLIB(PDPEPIL),DISP=(OLD,PASS)
//SYSUT1   DD  *
undivert(pdpepil.mac)dnl
/*
//SYSPRINT DD  SYSOUT=*
//SYSIN    DD  DUMMY
//*
//MVSSTART EXEC PDPASM
//SYSIN  DD  *
undivert(mvsstart.asm)dnl
/*
//MVSSUPA  EXEC PDPASM,LIST='LIST'
//SYSIN  DD  *
undivert(mvssupa.asm)dnl
/*
//START    EXEC PDPASM
//SYSIN  DD *
undivert(start.s)dnl
/*
//STDIO    EXEC PDPASM
//SYSIN  DD *
undivert(stdio.s)dnl
/*
//STDLIB   EXEC PDPASM
//SYSIN  DD  *
undivert(stdlib.s)dnl
/*
//CTYPE    EXEC PDPASM
//SYSIN  DD  *
undivert(ctype.s)dnl
/*
//STRING   EXEC PDPASM
//SYSIN  DD  *
undivert(string.s)dnl
/*
//TIME     EXEC PDPASM
//SYSIN  DD  *
undivert(time.s)dnl
/*
//ERRNO    EXEC PDPASM
//SYSIN  DD  *
undivert(errno.s)dnl
/*
//ASSERT   EXEC PDPASM
//SYSIN  DD  *
undivert(assert.s)dnl
/*
//LOCALE   EXEC PDPASM
//SYSIN  DD  *
undivert(locale.s)dnl
/*
//MATH     EXEC PDPASM
//SYSIN  DD  *
undivert(math.s)dnl
/*
//SETJMP   EXEC PDPASM
//SYSIN  DD  *
undivert(setjmp.s)dnl
/*
//SIGNAL   EXEC PDPASM
//SYSIN  DD  *
undivert(signal.s)dnl
/*
//@@MEMMGR EXEC PDPASM
//SYSIN  DD  *
undivert(__memmgr.s)dnl
/*
//
