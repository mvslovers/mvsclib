**********************************************************************
*                                                                    *
*  THIS PROGRAM WRITTEN BY PAUL EDWARDS.                             *
*  RELEASED TO THE PUBLIC DOMAIN                                     *
*                                                                    *
**********************************************************************
**********************************************************************
*                                                                    *
*  VSESTART - STARTUP ROUTINES FOR VSE FOR USE WITH GCC.             *
*                                                                    *
*  This code is able to handle z/VSE-style parameters. It saves the  *
*  UPSI as well for later use by the C code.  The stack is allocated *
*  here, below the line                                              *
*                                                                    *
**********************************************************************
         COPY  PDPTOP
         PRINT GEN
* REGEQU IS NOT AVAILABLE ON DOS/VS
*         YREGS
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
SUBPOOL  EQU   0
         CSECT
*
* Put an eyecatcher here to ensure program has been linked
* correctly.
         DC    C'PDPCLIB!'
         ENTRY @@CRT0
@@CRT0   DS    0H
*         ENTRY CEESTART
*CEESTART DS    0H
*
* DOS/VS doesn't require or allow saving of registers
*         SAVE  (14,12),,@@CRT0
*
         BALR  R10,R0
         LA    R10,0(R10)        clean address
         BCTR  R10,0
         BCTR  R10,0
         USING @@CRT0,R10
*
         LR    R8,R15            save R15 so that we can get the PARM
         LR    R11,R1            save R1 so we can get the PARM
         GETVIS LENGTH=STACKLEN
         LTR   R15,R15
         BNZ   NOMEM  
         ST    R13,4(R1)
         LR    R13,R1
         USING STACK,R13
*
         LA    R2,0
         ST    R2,DUMMYPTR       WHO KNOWS WHAT THIS IS USED FOR
         LA    R2,MAINSTK
         ST    R2,THEIRSTK       NEXT AVAILABLE SPOT IN STACK
         LA    R12,ANCHOR
         ST    R14,EXITADDR
         L     R3,=A(MAINLEN)
         AR    R2,R3
         ST    R2,12(R12)        TOP OF STACK POINTER
         LA    R2,0
         ST    R2,116(R12)       ADDR OF MEMORY ALLOCATION ROUTINE
*
* Now let's get the parameter list
*
         COMRG                   get address of common region in R1
         LR    R5,R1             use R5 to map common region
         USING COMREG,R5         address common region
         L     R2,SYSPAR         get access to SYSPARM
         LA    R2,0(R2)          clean the address, just in case
         ST    R2,ARGPTR         store SYSPARM
         MVC   XUPSI,UPSI        move the UPSI switches in
         LA    R2,0              default no VSE-style PARM
         CR    R11,R8            compare original R15 and original R1
         BE    CONTPARM          no difference = no VSE-style PARM
         LR    R2,R11            R11 has PARM, now R2 does too
* Note - do not clean the PARM, as we need access to the top bit
CONTPARM DS    0H
         ST    R2,ARGPTRE        store VSE-style PARM
         L     R2,JAPART         address of job accounting table
         USING ACCTABLE,R2       address accounting table
         MVC   PGMNAME,ACCTEXEC  move program name into our own area
         DROP  R5                no longer need common region
         LA    R2,PGMNAME        address our program name
         ST    R2,PGMNPTR        store program name
         DROP  R2                no longer need accounting table
*
* Set R4 to true if we were called in 31-bit mode
*
         LA    R4,0
         AIF   ('&ZSYS' EQ 'S370').NOBSM
         BSM   R4,R0
.NOBSM   ANOP
         ST    R4,SAVER4
*
* FOR GCC WE NEED TO BE ABLE TO RESTORE R13
         LA    R5,SAVEAREA
         ST    R5,SAVER13
*
         LA    R1,PARMLIST
*
         AIF   ('&ZSYS' NE 'S380').N380ST1
* If we were called in AMODE 31, don't bother setting mode now
         LTR   R4,R4
         BNZ   IN31
         CALL  @@SETM31
IN31     DS    0H
.N380ST1 ANOP
*
         CALL  @@START
         LR    R9,R15
*
         AIF   ('&ZSYS' NE 'S380').N380ST2
* If we were called in AMODE 31, don't switch back to 24-bit
         LTR   R4,R4
         BNZ   IN31B
         CALL  @@SETM24
IN31B    DS    0H
.N380ST2 ANOP
*
RETURNMS DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R14,R9
         FREEVIS LENGTH=STACKLEN
         LR    R15,R14
         EOJ
NOMEM    DS    0H
         EXCP  CCB
         WAIT  CCB
         EOJ
ERRMSG   DC    C'PDPCLIB HAS INSUFFICIENT MEMORY - NO SIZE=AUTO?'
CCB      CCB   SYSLOG,CCW
CCW      CCW   X'09',ERRMSG,0,L'ERRMSG
SAVER4   DS    F
SAVER13  DS    F
         LTORG
*         ENTRY CEESG003
*CEESG003 DS    0H
         DS    0H
         ENTRY @@EXITA
@@EXITA  DS    0H
* SWITCH BACK TO OUR OLD SAVE AREA
         LR    R10,R15
         USING @@EXITA,R10
         L     R9,0(R1)
         L     R13,=A(SAVER13)
         L     R13,0(R13)
*
         AIF   ('&ZSYS' NE 'S380').N380ST3
         L     R4,=A(SAVER4)
         L     R4,0(R4)
* If we were called in AMODE 31, don't switch back to 24-bit
         LTR   R4,R4
         BNZ   IN31C
         CALL  @@SETM24
IN31C    DS    0H
.N380ST3 ANOP
*
         LR    R1,R13
         L     R13,4(R13)
         LR    R14,R9
         FREEVIS LENGTH=STACKLEN
         LR    R15,R14
*         RETURN (14,12),RC=(15)
         EOJ
         LTORG
STACKLEN DC    A(STKLTMP)
*
STACK    DSECT
SAVEAREA DS    18F
DUMMYPTR DS    F
THEIRSTK DS    F
PARMLIST DS    0F
ARGPTR   DS    F
PGMNPTR  DS    F
ARGPTRE  DS    F
TYPE     DS    F
PGMNAME  DS    CL8
PGMNAMEN DS    C                 NUL BYTE FOR C
XUPSI    DS    C                 UPSI switches
ANCHOR   DS    0F
EXITADDR DS    F
         DS    49F
MAINSTK  DS    32000F
MAINLEN  EQU   *-MAINSTK
STKLTMP  EQU   *-STACK
*
COMREG   MAPCOMR
*
* MAPACCT - map ACCTABLE - from DOS/VS supervisor
*
ACCTABLE DSECT                    PARTITION ACCOUNTING TABLE
ACCTWK1  DS    F                  WORK AREAS
ACCTWK2  DS    F
ACCTSVPT DS    F                       SAVE AREA FOR JOB CARD PTR
ACCTPART DS    X                       PARTITION SWITCH KEY
ACCTRES2 DS    X                       RESERVED
ACCTLEN  DS    H                  LENGTH OF SIO PART OF TABLE.
ACCTLOAD DS    3H                      INST TO SET LABEL AREA
ACCTRES3 DS    H                       RESERVED
ACCTLADD DS    A                       ADDR OF LABEL AREA
ACCTCPUT DS    F                       PARTITION CPU TIME COUNTER
ACCTOVHT DS    F                       OVERHEAD CPU COUNTER
ACCTBNDT DS    F                       WAIT TIME COUNTER
ACCTSVJN DS    CL8                     JOB NAME SAVE AREA
*        REG 15 HAS ADDRESS OF FOLLOWING LABEL WHEN $JOBACCT CALLED.
ACCTJBNM DS    CL8                START OF USERS SECTION OF TABLE.
ACCTUSRS DS    CL16               USERS ACCOUNT INFORMATION
ACCTPTID DS    CL2                PARTITION ID
ACCTCNCL DS    XL1                CANCEL CODE FOR JOB STEP
ACCTYPER DS    XL1                TYPE OF ACCOUNTING RECORD
ACCTDATE DS    CL8                DATE OF JOB
ACCTSTRT DS    F                  JOB START TIME
ACCTSTOP DS    F                  JOB STOP TIME
ACCTRESV DS    F                  RESERVED
ACCTEXEC DS    CL8                JOB STEP PHASE NAME
ACCTHICR DS    F                  JOB STEP HI-CORE ADDRESS
ACCTIMES DS    3F                 EXECUTION TIME BREAKDOWN FIELDS
ACCTSIOS DS    X                  START OF SIO TABLE
         END
