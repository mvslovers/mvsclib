***********************************************************************
*                                                                     *
*  This program written by Paul Edwards.                              *
*  Released to the public domain                                      *
*                                                                     *
*  Contributions from Louis Millon et al also public domain.          *
*                                                                     *
***********************************************************************
*                                                                     *
*  VSESUPA - Support routines for PDPCLIB under DOS/VSE               *
*                                                                     *
*  This assembler code has a long history - starting off as C/370     *
*  under MVS then modified for GCC, then ported to CMS, MUSIC/SP      *
*  and then finally VSE. A rewrite by someone with deep VSE           *
*  experience should be considered. Also, it will probably be         *
*  necessary at some point to introduce a flag to say whether it is   *
*  being built for z/VSE or DOS/VS R34, as new functionality like     *
*  the use of LABEL/LPL for dynamic file allocation shouldn't be      *
*  held back by the older DOS/VS. Perhaps dummy macros can be         *
*  created rather than a flag, but either way, z/VSE should never be  *
*  seriously compromised.                                             *
*                                                                     *
***********************************************************************
*
*
* LDINT macro
*
         MACRO ,             COMPILER DEPENDENT LOAD INTEGER
&NM      LDINT &R,&A         LOAD INTEGER VALUE FROM PARM LIST
         GBLC  &COMP         COMPILER GCC OR IBM C
&NM      L     &R,&A         LOAD PARM VALUE
         AIF ('&COMP' EQ 'GCC').MEND
.* THIS LINE IS FOR ANYTHING NOT GCC: IBM C
         L     &R,0(,&R)     LOAD INTEGER VALUE
.MEND    MEND  ,
*
*
*
         COPY  PDPTOP
*
         CSECT
         PRINT GEN
* REGEQU is not standard
*         REGEQU
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
*
*
***********************************************************************
*                                                                     *
*  AOPEN - Open a file                                                *
*                                                                     *
*  Parameters are:                                                    *
*  DDNAME - space-padded, 8 character DDNAME to be opened             *
*    Note that in VSE, the DDNAME may be expanded to be more than 8   *
*    bytes. It represents not just the 7-character filename on the    *
*    DLBL, but may also include recfm, lrecl and blksize info.        *
*    It is not expected that there is much call for that though,      *
*    so there are reasonable defaults.                                *
*    First we have a fairly fixed portion - e.g. SDO1, which is the   *
*    disk label, which is also the macro name, the typefle (I or O)   *
*    of the macro, and it's sequence number (currently statically     *
*    defined, but potentially it will be dynamic).                    *
*    All files are defined as RECFM=U, since that gives the program   *
*    the flexibility to decide how to treat it, and in DOS there is   *
*    no-one else who will disagree, with the info not stored in the   *
*    VTOC or catalog or DCB or anywhere else.                         *
*    The next bit of the DDNAME says how you want the file to be      *
*    internally treated. The default is RECFM=U which for input files *
*    is the maximum possible for a 3350, but on output it is a        *
*    figure that is more flexible for the sort of data that may be    *
*    stored - 6480 - a multiple of both 80 and 81, that fits on most  *
*    disk types, while still being over 90% efficient on a 3390.      *
*    Otherwise the user may specify FB80 which will treat the data    *
*    as F80 records, blocked to 6480, which depending on other things *
*    may trigger breakdown of records, stripping of blanks etc.       *
*    For output only, F80 may be specified to force the data to be    *
*    unblocked. It has no meaning (and is invalid) on input.          *
*    A similar situation exists for tapes.                            *
*    That's the theoretical input - sort of like an SDI1,FB,80,6480   *
*    In practice we don't yet have the ability to deblock, so any     *
*    FB80 file must in actual fact be F80.                            *
*  MODE - 0 = READ, 1 = WRITE, 2 = UPDATE (update not supported)      *
*  RECFM - 0 = F, 1 = V, 2 = U. This is an output from this function  *
*  LRECL - This function will determine the LRECL                     *
*  BLKSIZE - This function will determine the block size              *
*  ASMBUF - pointer to a 32K area which can be written to (only       *
*    needs to be set in move mode)                                    *
*  MEMBER - *pointer* to space-padded, 8 character member name.       *
*    If pointer is 0 (NULL), no member is requested                   *
*                                                                     *
*  Return value:                                                      *
*  An internal "handle" that allows the assembler routines to         *
*  keep track of what's what when READ etc are subsequently           *
*  called.                                                            *
*                                                                     *
*                                                                     *
*  Note - more documentation for this and other I/O functions can     *
*  be found halfway through the stdio.c file in PDPCLIB.              *
*                                                                     *
*                                                                     *
*                                                                     *
*  In the general case of an open of a disk file, ideally the         *
*  OPEN should allocate its storage area (ZDCBAREA - what "handle"    *
*  points to, and then it should copy the DTFSD into part of that     *
*  "DCB area" (it is called that for historical reasons and will      *
*  probably be renamed). The OPEN macro, using register notation,     *
*  points to that area, which will have first been modified to put    *
*  in the DDNAME (DLBL) being opened. This way we only need a         *
*  single DTFSD in the main code, which is reused any number of       *
*  times. However, at the moment we have simply assumed a small       *
*  number of files, which is sufficient to allow a C compile to go    *
*  through.                                                           *
*                                                                     *
*  The stdin/stdout/stderr are treated differently - each of those    *
*  has its own DTF, because they are special files (not disks).       *
*  The special files are SYSIPT, SYSLST and SYSLOG respectively.      *
*                                                                     *
*  Another technique that has been used is for accessing members of   *
*  a PDS - they are assumed to be in the CIL, and loaded, then        *
*  data is read from them as if it was a RECFM=U dataset.             *
*  It is expected that this technique will be expanded in the future  *
*  to also allow a similar operation from a source statement          *
*  library.                                                           *
*                                                                     *
*  Also note that the C code is totally flexible in that it will      *
*  do whatever this assembler code tells it to. ie you can set any    *
*  file to any RECFM/LRECL and it will do its work based on that.     *
*  This makes it possible to change anything in here that isn't       *
*  working to your satisfaction, without needing to change the C      *
*  code at all.                                                       *
*                                                                     *
***********************************************************************
         ENTRY @@AOPEN
@@AOPEN  DS    0H
         SAVE  (14,12),,@@AOPEN
         LR    R12,R15
         USING @@AOPEN,R12
         LR    R11,R1
         L     R0,=A(ZDCBLEN)
         AIF   ('&ZSYS' EQ 'S390').BELOW
* USE DEFAULT LOC=RES for S/370 and S/380
         GETVIS
         AGO   .CHKBLWE
.BELOW   ANOP
         GETVIS LOC=BELOW
.CHKBLWE ANOP
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R3,0(R1)           R3 POINTS TO DDNAME
         L     R6,4(R1)
         LDINT R6,0(R6)           R6 now has value of mode
* 08(,R1) has RECFM
* Note that R5 is used as a scratch register
         L     R8,12(,R1)         R8 POINTS TO LRECL
* 16(,R1) has BLKSIZE
* 20(,R1) has ASMBUF pointer
*
         LA    R9,0
         ST    R9,ISMEM
         ST    R9,ISDI
         L     R9,24(,R1)         R9 POINTS TO MEMBER NAME (OF PDS)
         LA    R9,0(,R9)          Strip off high-order bit or byte
*
         LR    R2,R13             Access DCB
         LA    R2,WORKLEN(R2)     Point past save area
         LR    R0,R2              Load output DCB area address
         L     R1,=A(ZDCBLEN)     Load output length of DCB area
         S     R1,=A(WORKLEN)     Adjust for save area
         LR    R5,R11             Preserve parameter list
         LA    R11,0              Pad of X'00' and no input length
         MVCL  R0,R10             Clear DCB area to binary zeroes
         LR    R11,R5             Restore parameter list
* R5 free again
*
*
         LTR   R6,R6
         BNZ   WRITING
*
* So now we're doing the reading code
*
* Something like RDJFCB would be good here, if VSE has such a thing
*
         LTR   R9,R9
         BZ    NOMEM
*
* Although VSE doesn't have PDSes with members, it has something
* similar - libraries. It is actually the Core Image library that
* is the most flexible, allowing binary data to be stored.
* Unfortunately this can't be directly read or written! But what
* we can do is use LNKEDT to build a module, then load it later,
* to be read as a file.
*
         ST    R9,ISMEM
         MVC   MEMBER24,0(R9)
         LA    R9,=C'OPEN    '
         ST    R9,P1VF
         LA    R9,MEMBER24
         ST    R9,P2VF
         LA    R1,PMVF
         CALL  @@VSEFIL
*
* We should be able to have 32k records here
         L     R6,=F'19069'   +++ hardcode to 19069
         ST    R6,DCBLRECL
         LA    R6,2           +++ hardcode to recfm=U
         ST    R6,DCBRECFM
         B     DONEOPEN
NOMEM    DS    0H
*
* Normal datasets just need to be opened - but unfortunately
* we don't know what their DCB info is. What we basically do
* to get around that problem is to hardcode DCB info based on
* the DDNAME. There are various techniques that could be used
* to work around this limitation, and one should be implemented.
*
* We use the register notation, because other than the standard
* files, all files will read/write data from a field in ZDCBAREA
* rather than a variable defined in this CSECT.
*
         CLC   0(8,R3),=C'SYSIN   '
         BNE   NOTSYSI
         LA    R6,80          +++ hardcode to 80
         ST    R6,DCBLRECL
         LA    R6,0           +++ hardcode to fixed
         ST    R6,DCBRECFM
         LA    R6,1
         ST    R6,ISDI   sysin is device-independent
         LA    R5,SYSIN
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
NOTSYSI  DS    0H
*
* All other files currently defined are RECFM=U
*
         L     R6,=F'19069'   +++ hardcode to 19069
         ST    R6,DCBLRECL
         LA    R6,2           +++ hardcode to recfm=U
         ST    R6,DCBRECFM
*
* Here we need to choose tape or disk
* There's probably a better way than looking at the name of
* the DD, to see if it starts with "MT", as a convention, 
* but of course it would be better if this was
* transparent to the programmer in the first place!
*
         CLC   0(2,R3),=C'MT'
         BNE   NOTTAP
         LA    R5,MTI1
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
NOTTAP   DS    0H
*
* Need to allow more input files, and DCB info
*
         CLC   0(8,R3),=C'SDI1FB80'
         BNE   NFB80I1
*
* Warning - either this assembler code, or the calling C program,
* should be made sophisticated enough to handle FB. But at the
* moment, such deblocking is not available, so although the
* syntax caters for FB, we actually only support F.
*
         LA    R6,0           +++ hardcode to recfm=F
         ST    R6,DCBRECFM
         L     R6,=F'80'      +++ hardcode to 80
         ST    R6,DCBLRECL
NFB80I1  DS    0H
         LA    R5,SDI1
         CLC   0(4,R3),=C'SDI1'
         BE    GOTSDI1
         LA    R5,SDI2        +++ assume SDI2
GOTSDI1  DS    0H
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
* Can't reach here, since all files are currently considered valid
         B     BADOPEN
*
*
*
WRITING  DS    0H
*
* Would be good if we could do a RDJFCB here to get DCB info.
* Instead, we just assume it from the DD name.
*
* Writing to a member of a library is not directly supported in VSE,
* and the workaround for this situation is done outside of this
* assembler code, so nothing to see here folks!
*
* We use the register notation, because other than the standard
* files, all files will read/write data from a field in ZDCBAREA
* rather than a variable defined in this CSECT.
*
WNOMEM   DS    0H
         CLC   0(8,R3),=C'SYSPRINT'
         BNE   NOTSYSPR
         LA    R6,120         lrecl = 120
         ST    R6,DCBLRECL
         LA    R6,0           recfm = fixed
         ST    R6,DCBRECFM
         LA    R6,1
         ST    R6,ISDI   sysprint is device-independent
         L     R6,DCBLRECL
         LA    R5,SYSPRT
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
NOTSYSPR DS    0H
         CLC   0(8,R3),=C'SYSTERM '
         BNE   NOTSYST
         LA    R6,80          +++ hardcode to 80
         ST    R6,DCBLRECL
         LA    R6,0           +++ hardcode to fixed
         ST    R6,DCBRECFM
         L     R6,DCBLRECL
         LA    R5,SYSTRM
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
NOTSYST  DS    0H
         CLC   0(8,R3),=C'SYSPUNCH'
         BNE   NOTSYSPU
         LA    R6,80          lrecl = 80
         ST    R6,DCBLRECL
         LA    R6,0           recfm = fixed
         ST    R6,DCBRECFM
         LA    R6,1
         ST    R6,ISDI   syspunch is device-independent
         L     R6,DCBLRECL
         LA    R5,SYSPCH
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
NOTSYSPU DS    0H
*
* We should really make this smart enough to be allocated to
* any SDO file, and for it to pick up the LRECL as well.
*
         CLC   0(8,R3),=C'SDO1F80 '
         BNE   NOTF80O1
         LA    R6,80          +++ hardcode to 80
         ST    R6,DCBLRECL
         LA    R6,0           +++ hardcode to fixed
         ST    R6,DCBRECFM
         L     R6,DCBLRECL
         LA    R5,SDO1
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
NOTF80O1 DS    0H
*
* Assume RECFM=U
* Note that output files can't really use up to the full 19069
* and 18452 is a better match for a 3390 anyway. However, this
* is set to 80 * 81 for ease of use by other programs.
* Also note that we need to cater for SDO2 etc too
*
         L     R6,=F'6480'    +++ hardcode to 6480
         ST    R6,DCBLRECL
         LA    R6,2           +++ hardcode to undefined
         ST    R6,DCBRECFM
         L     R6,DCBLRECL
*
*
* Here we need to choose tape or disk. Actually only MTO1 is
* currently supported, but we don't check for that.
*
         CLC   0(2,R3),=C'MT'
         BNE   NOTTAPW
         LA    R5,MTO1
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
*
*
*
NOTTAPW  DS    0H
         LA    R5,SDO1
         ST    R5,PTRDTF
         OPEN  (R5)
         B     DONEOPEN
* Can't reach here, since all files are currently considered valid
         B     BADOPEN
*
*
*
*
DONEOPEN DS    0H
*
* We've done the open (read or write), and now need to allocate a 
* buffer that the C code can write to (or in the case of read,
* that the assembler code can use). The buffer needs to be below the
* line, so it's simpler if the assembler code allocates it on
* behalf of the C caller. We should really allocate a buffer size
* based on what is actually required rather than this hardcoded
* maximum possible.
*
         L     R6,=F'32768'
*
* S/370 can't handle LOC=BELOW
*
         AIF   ('&ZSYS' EQ 'S390').MVT8090  If not 390
         GETVIS LENGTH=(R6)  Use default LOC=RES for S/370 and S/380
         AGO   .GETOENE
.MVT8090 ANOP  ,                  S/390
         GETVIS LENGTH=(R6),LOC=BELOW
.GETOENE ANOP
*
* Give this buffer pointer back to caller
         ST    R1,ASMBUF
         L     R5,20(,R11)        R5 points to ASMBUF
         ST    R1,0(R5)           save the pointer
* Note that in the case of read, the caller doesn't need to know
* the address (something appropriate is returned in the read
* function - and appropriate means that the assembler may have
* deblocked the records and be pointing to that), but it seems 
* harmless to set the value anyway.
*
*
* Set other values that the caller needs to know
*
* The LRECL
         L     R6,DCBLRECL
         ST    R6,0(R8)
* The RECFM
         L     R6,DCBRECFM
         L     R5,8(,R11)         Point to RECFM
         ST    R6,0(R5)
* Now return success
         B     RETURNOP
*
*
* We failed to open the file, so free the allocated memory and
* return an error.
*
BADOPEN  DS    0H
         L     R0,=A(ZDCBLEN)
         LR    R1,R13
         L     R7,SAVEAREA+4
         FREEVIS
         L     R15,=F'-1'
         LR    R13,R7
         RETURN (14,12),RC=(15)
*
*
* Good return - handle is in ZDCBAREA, which is R13. So we don't
* want to free that memory!
*
RETURNOP DS    0H
         LR    R15,R13
         L     R13,SAVEAREA+4
         RETURN (14,12),RC=(15)
         LTORG
*
*
***********************************************************************
*                                                                     *
*  AREAD - Read from file                                             *
*                                                                     *
*  This function takes 3 parameters:                                  *
*                                                                     *
*  1. A handle (previously returned by AOPEN)                         *
*  2. A buffer pointer - this is an output variable - the assembler   *
*     routine will read the data and then inform the caller where     *
*     the data is located.                                            *
*  3. Length of data (also output).                                   *
*                                                                     *
***********************************************************************
         ENTRY @@AREAD
@@AREAD  DS    0H
         SAVE  (14,12),,@@AREAD
         LR    R12,R15
         USING @@AREAD,R12
         LR    R11,R1
*
         AIF ('&ZSYS' EQ 'S370').NOMOD1
         CALL  @@SETM24
.NOMOD1  ANOP
*
         L     R1,0(R1)         R1 CONTAINS HANDLE
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
         L     R3,4(R1)         R3 POINTS TO BUF POINTER
         L     R4,8(R1)         R4 points to a length
*
* See if this is a library file
*
         L     R9,ISMEM
         LTR   R9,R9
         BNZ   GMEM
*
* For non-library files, we read into an internal buffer that
* was allocated earlier and is pointed to by the zdcbarea. Set 
* that fact immediately.
*
         L     R5,ASMBUF
         ST    R5,0(R3)
*
* The DTF macro is expecting to get the maximum length in R8
*
         L     R8,DCBLRECL
         L     R7,PTRDTF
*
         L     R9,ISDI            Is this device-independent?
         LTR   R9,R9
         BNZ   GDIR
*
* Normal file. GET needs the DTF pointer, the buffer, and our
* DTF is expecting the length in R8
*
         GET   (R7),(R5)
         B     DONEGET
*
* Got a device-indepentent DTF - we only support a RECSIZE of 80,
* and do not support a file containing an intiial control 
* character, so if you have such a file you will need to trim
* it down using a separate utility first.
*
GDIR     DS    0H          Got a device-independent
         GET   (R7)
         LA    R8,80     +++ hardcoded length of 80
         MVC   0(80,R5),IO1  +++ hardcode IO1 and length
DONEGET  DS    0H
* If GET reaches EOF, the "GOTEOF" label will be branched to
* automatically.
         LA    R15,0             SUCCESS
         ST    R8,0(R4)          store length actually read
         B     FINFIL
*
* This is a library file, so we need to call VSEFIL
*
GMEM     DS    0H                got member
         LA    R9,=C'GET     '
         ST    R9,P1VF
         LA    R9,MEMBER24
         ST    R9,P2VF
* Let VSEFIL directly set our caller's parameters
         ST    R3,P3VF
         ST    R4,P4VF
         LA    R1,PMVF
         CALL  @@VSEFIL
         L     R9,0(R4)
         LTR   R9,R9
         BNZ   FINFIL
         B     GOTEOF
GOTEOF   DS    0H
         LA    R15,1             FAIL
FINFIL   DS    0H
*
RETURNAR DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
*
         AIF ('&ZSYS' EQ 'S370').NOMOD2
         LR    R7,R15            Preserve R15 over call
         CALL  @@SETM31
         LR    R15,R7
.NOMOD2  ANOP
*
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
***********************************************************************
*                                                                     *
*  AWRITE - Write to file                                             *
*                                                                     *
*  This function takes 3 parameters:                                  *
*                                                                     *
*  1. A handle (previously returned by AOPEN)                         *
*  2. Address of buffer to be written (also previously obtained       *
*     from AOPEN).                                                    *
*  3. Length of data to be written (which may be ignored for a file   *
*     that is of an expected length, e.g. fixed 80)                   *
*                                                                     *
***********************************************************************
         ENTRY @@AWRITE
@@AWRITE DS    0H
         SAVE  (14,12),,@@AWRITE
         LR    R12,R15
         USING @@AWRITE,R12
         LR    R11,R1             SAVE
*
         AIF   ('&ZSYS' NE 'S380').N380WR1
         CALL  @@SETM24
.N380WR1 ANOP
*
         L     R1,0(R1)           R1 IS NOW HANDLE
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11             RESTORE
         USING WORKAREA,R13
*
         L     R2,0(,R1)          R2 contains GETMAINed address
         L     R3,4(,R1)          R3 points to the record address
         L     R3,0(,R3)          R3 now has actual buffer address
         L     R8,8(,R1)          R8 points to the length
         L     R8,0(,R8)          R8 now has actual length
*
         L     R5,PTRDTF
         L     R9,ISDI            Is this device-independent?
         LTR   R9,R9
         BNZ   GDIW
*
* Normal file. PUT needs the DTF pointer, the buffer, and our
* DTF is expecting the length in R8 (unless the DTF is fixed)
*
         PUT   (R5),(R3)
         B     DONEPUT
*
* Got a device-independent DTF (which requires a control character)
*
GDIW     DS    0H
         EX    R8,WRMOVE
         MVI   IO1,C' '          space seems universal rather than V/W
         PUT   (R5)
*
* We have written to file, but should really check for any error
*
DONEPUT  DS    0H
*
         AIF   ('&ZSYS' NE 'S380').N380WR2
         CALL  @@SETM31
.N380WR2 ANOP
*
         L     R13,4(R13)
         LA    R15,0             +++ hardcode success
         RETURN (14,12),RC=(15)
WRMOVE   MVC   IO1+1(0),0(R3)    +++ hardcode IO1
         LTORG
*
***********************************************************************
*                                                                     *
*  ACLOSE - Close file                                                *
*                                                                     *
*  This routine takes a single parameter - a handle as given by the   *
*  (successful) return from AOPEN.                                    *
*                                                                     *
***********************************************************************
         ENTRY @@ACLOSE
@@ACLOSE DS    0H
         SAVE  (14,12),,@@ACLOSE
         LR    R12,R15
         USING @@ACLOSE,R12
         LR    R11,R1           SAVE
*
* The CLOSE appears to be abending when called in 31-bit mode,
* despite it being an SVC. So we need to switch to 24-bit mode
         AIF   ('&ZSYS' NE 'S380').N380CL1
         CALL  @@SETM24
.N380CL1 ANOP
*
         L     R1,0(R1)         R1 CONTAINS HANDLE
         ST    R13,4(R1)
         ST    R1,8(R13)
         LR    R13,R1
         LR    R1,R11
         USING WORKAREA,R13
*
*
         L     R5,ASMBUF
         LTR   R5,R5
         BZ    NFRCL
         L     R6,=F'32768'     +++ hardcode length of ASMBUF
         FREEVIS LENGTH=(R6),ADDRESS=(R5)
NFRCL    DS    0H
*
*
         L     R5,PTRDTF        Get DTF
         LTR   R5,R5
         BZ    NOTOPEN
         L     R9,ISMEM         Is this a library member?
         LTR   R9,R9
         BNZ   GMEM2
*
* Normal file - just do a close.
*
         CLOSE (R5)
         B     DONECLOS
*
* We have a library member, so call VSEFIL to close
*
GMEM2    DS    0H
         LA    R9,=C'CLOSE   '
         ST    R9,P1VF
         LA    R9,MEMBER24
         ST    R9,P2VF
         LA    R1,PMVF
         CALL  @@VSEFIL
         B     DONECLOS
*
* We probably shouldn't have a specific detection for a close
* of a file that is not open, as it's a logic error regardless.
*
NOTOPEN  DS    0H
         LA    R15,1
         B     RETURNAC
*
* We should have some error detection here, but for now, just
* set success unconditionally
*
DONECLOS DS    0H
         LA    R15,0
*
RETURNAC DS    0H
         LR    R1,R13
         L     R13,SAVEAREA+4
         LR    R7,R15
         L     R0,=A(ZDCBLEN)
         FREEVIS
*
         AIF   ('&ZSYS' NE 'S380').N380CL2
         CALL  @@SETM31
.N380CL2 ANOP
*
         LR    R15,R7
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
* Note that a lot of these macros use the same storage buffer,
* because by their nature, the C caller will always read or
* write an entire block at a time.
*
*
* This is for reading from stdin
SYSIN    DTFDI DEVADDR=SYSIPT,IOAREA1=IO1,RECSIZE=80,EOFADDR=GOTEOF
*
* This is for writing to SYSPUNCH in a device-independent manner
* Note that it is a requirement to allow for a control character
SYSPCH   DTFDI DEVADDR=SYSPCH,IOAREA1=IO1,RECSIZE=81
*
* This is for writing to stdout (SYSPRINT)
SYSPRT   DTFDI DEVADDR=SYSLST,IOAREA1=IO1,RECSIZE=121
*
* This is for writing to stderr (SYSTERM)
SYSTRM   DTFPR CONTROL=YES,BLKSIZE=80,DEVADDR=SYS005,MODNAME=PRINTMOD, X
               IOAREA1=IO1,RECFORM=FIXUNB,WORKA=YES
PRINTMOD PRMOD CONTROL=YES,RECFORM=FIXUNB,WORKA=YES
*
* This is for reading from a sequential disk file
SDI1     DTFSD BLKSIZE=19069,DEVADDR=SYS000,DEVICE=3350,               X
               IOAREA1=WORKI1,RECFORM=UNDEF,WORKA=YES,                 X
               TYPEFLE=INPUT,RECSIZE=(8),EOFADDR=GOTEOF
*
* Another SD
SDI2     DTFSD BLKSIZE=19069,DEVADDR=SYS000,DEVICE=3350,               X
               IOAREA1=WORKI1,RECFORM=UNDEF,WORKA=YES,                 X
               TYPEFLE=INPUT,RECSIZE=(8),EOFADDR=GOTEOF
*
* This is for writing to a sequential disk file
SDO1     DTFSD BLKSIZE=19069,DEVICE=3350,                              X
               IOAREA1=WORKO1,RECFORM=UNDEF,WORKA=YES,                 X
               TYPEFLE=OUTPUT,RECSIZE=(8)
*
* This is for reading from a tape
MTI1     DTFMT BLKSIZE=19069,DEVADDR=SYS011,MODNAME=MTMOD,             X
               IOAREA1=WORKI1,RECFORM=UNDEF,WORKA=YES,FILABL=NO,       X
               TYPEFLE=INPUT,RECSIZE=(8),EOFADDR=GOTEOF
*
* This is for writing to a tape
MTO1     DTFMT BLKSIZE=19069,DEVADDR=SYS011,MODNAME=MTMOD,             X
               IOAREA1=WORKO1,RECFORM=UNDEF,WORKA=YES,FILABL=STD,      X
               TYPEFLE=OUTPUT,RECSIZE=(8)
*
* For some reason this MOD can be shared by both input and
* output, and in fact, there's not much choice, because otherwise
* we get duplicate symbols.
MTMOD    MTMOD WORKA=YES,RECFORM=UNDEF
*
* For the standard files, this is sufficient for input and output
IO1      DS    CL200
*
*
*
* This is pretty crappy - storing large variables in the CSECT
* instead of the dynamically-allocated DSECT. But at least the
* fact that they are being shared makes it not so bad.
* An extra 100 bytes to be helpful.
WORKI1   DS    CL32767
WORKO1   DS    CL32767
*
*
***********************************************************************
*                                                                     *
*  GETM - GET MEMORY                                                  *
*                                                                     *
***********************************************************************
         ENTRY @@GETM
@@GETM   DS    0H
         SAVE  (14,12),,@@GETM
         LR    R12,R15
         USING @@GETM,R12
*
         LDINT R3,0(,R1)          LOAD REQUESTED STORAGE SIZE
         LR    R4,R3
         LA    R3,8(,R3)
*
* To avoid fragmentation, round up size to 64 byte multiple
*
         A     R3,=A(64-1)
         N     R3,=X'FFFFFFC0'
*
         AIF   ('&ZSYS' NE 'S380').N380GM1
*
* When in 380 mode, we need to keep the program below the
* line, but we have the ability to use storage above the
* line, and this is where we get it, with the LOC=ANY parameter.
* For other environments, the default LOC=RES is fine.
*
         GETVIS LENGTH=(R3),LOC=ANY
         AGO   .N380GM2
.N380GM1 ANOP
         GETVIS LENGTH=(R3)
.N380GM2 ANOP
*
         LTR   R15,R15
         BZ    GOODGM
         LA    R15,0
         B     RETURNGM
GOODGM   DS    0H
* WE STORE THE AMOUNT WE REQUESTED FROM VSE INTO THIS ADDRESS
         ST    R3,0(R1)
* AND JUST BELOW THE VALUE WE RETURN TO THE CALLER, WE SAVE
* THE AMOUNT THEY REQUESTED
         ST    R4,4(R1)
         A     R1,=F'8'
         LR    R15,R1
*
RETURNGM DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
***********************************************************************
*                                                                     *
*  FREEM - FREE MEMORY                                                *
*                                                                     *
***********************************************************************
         ENTRY @@FREEM
@@FREEM  DS    0H
         SAVE  (14,12),,@@FREEM
         LR    R12,R15
         USING @@FREEM,R12
*
         L     R2,0(,R1)
         S     R2,=F'8'
         L     R3,0(,R2)
*
         FREEVIS LENGTH=(R3),ADDRESS=(R2)
*
RETURNFM DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
***********************************************************************
*                                                                     *
*  GETCLCK - GET THE VALUE OF THE MVS CLOCK TIMER AND MOVE IT TO AN   *
*  8-BYTE FIELD.  THIS 8-BYTE FIELD DOES NOT NEED TO BE ALIGNED IN    *
*  ANY PARTICULAR WAY.                                                *
*                                                                     *
*  E.G. CALL 'GETCLCK' USING WS-CLOCK1                                *
*                                                                     *
*  THIS FUNCTION ALSO RETURNS THE NUMBER OF SECONDS SINCE 1970-01-01  *
*  BY USING SOME EMPERICALLY-DERIVED MAGIC NUMBERS                    *
*                                                                     *
***********************************************************************
         ENTRY @@GETCLK
@@GETCLK DS    0H
         SAVE  (14,12),,@@GETCLK
         LR    R12,R15
         USING @@GETCLK,R12
*
         L     R2,0(,R1)
         STCK  0(R2)
         L     R4,0(,R2)
         L     R5,4(,R2)
         SRDL  R4,12
         SL    R4,=X'0007D910'
         D     R4,=F'1000000'
         SL    R5,=F'1220'
         LR    R15,R5
*
RETURNGC DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
**********************************************************************
*                                                                    *
*  GETAM - get the current AMODE                                     *
*                                                                    *
*  This function returns 24 if we are running in exactly AMODE 24,   *
*  31 if we are running in exactly AMODE 31, and 64 for anything     *
*  else (user-defined/infinity/16/32/64/37)                          *
*                                                                    *
*  Be aware that MVS 3.8j I/O routines require an AMODE of exactly   *
*  24 - nothing more, nothing less - so applications are required    *
*  to ensure they are in AM24 prior to executing any I/O routines,   *
*  and then they are free to return to whichever AMODE they were in  *
*  previously (ie anything from 17 to infinity), which is normally   *
*  done using a BSM to x'01', although this instruction was not      *
*  available in S/370-XA so much software does a BSM to x'80'        *
*  instead of the user-configurable x'01', which is unfortunate.     *
*                                                                    *
*  For traditional reasons, people refer to 24, 31 and 64, when what *
*  they should really be saying is 24, 31 and user-defined.          *
*                                                                    *
**********************************************************************
         ENTRY @@GETAM
@@GETAM  DS    0H
         SAVE  (14,12),,@@GETAM
         LR    R12,R15
         USING @@GETAM,R12
*
         L     R2,=X'C1800000'
         LA    R2,0(,R2)
         CLM   R2,B'1100',=X'0080'
         BE    GAIS24
         CLM   R2,B'1000',=X'41'
         BE    GAIS31
         LA    R15,64
         B     RETURNGA
GAIS24   DS    0H
         LA    R15,24
         B     RETURNGA
GAIS31   LA    R15,31
*
RETURNGA DS    0H
         RETURN (14,12),RC=(15)
         LTORG ,
         SPACE 2
*
*
*
***********************************************************************
*                                                                     *
*  LOAD - load a module into memory                                   *
*                                                                     *
*  parm1 = program name                                               *
*  parm2 = memory address                                             *
*                                                                     *
*  entry point address returned in R15                                *
*                                                                     *
*  Not currently used, but it might be!                               *
*                                                                     *
***********************************************************************
         ENTRY @@LOAD
@@LOAD   DS    0H
         SAVE  (14,12),,@@LOAD
         LR    R12,R15
         USING @@LOAD,R12
*
         L     R2,0(,R1)
         L     R3,4(,R1)
         LOAD  (R2),(R3)
         LR    R15,R1
*
RETURNLD DS    0H
         RETURN (14,12),RC=(15)
         LTORG
*
***********************************************************************
*                                                                     *
*  SYSTEM - execute another command                                   *
*                                                                     *
*  Not currently implemented, but ideally should be                   *
*                                                                     *
***********************************************************************
         ENTRY @@SYSTEM
@@SYSTEM DS    0H
         SAVE  (14,12),,@@SYSTEM
         LR    R12,R15
         USING @@SYSTEM,R12
         LR    R11,R1
*
*         GETVIS LENGTH=SYSTEMLN,SP=SUBPOOL
         ST    R13,4(,R1)
         ST    R1,8(,R13)
         LR    R13,R1
         LR    R1,R11
         USING SYSTMWRK,R13
*
         MVC   CMDPREF,FIXEDPRF
         L     R2,0(R1)
         CL    R2,=F'200'
         BL    LENOK
         L     R2,=F'200'
LENOK    DS    0H
         STH   R2,CMDLEN
         LA    R4,CMDTEXT
         LR    R5,R2
         L     R6,4(R1)
         LR    R7,R2
         MVCL  R4,R6
         LA    R1,CMDPREF
*         SVC   $EXREQ
*
RETURNSY DS    0H
         LR    R1,R13
         L     R13,SYSTMWRK+4
*         FREEMAIN RU,LV=SYSTEMLN,A=(1),SP=SUBPOOL
*
         LA    R15,0
         RETURN (14,12),RC=(15)
* For documentation on this fixed prefix, see SVC 221
* documentation.
FIXEDPRF DC    X'7F01E000000000'
         LTORG
SYSTMWRK DSECT ,             MAP STORAGE
         DS    18A           OUR OS SAVE AREA
CMDPREF  DS    CL8           FIXED PREFIX
CMDLEN   DS    H             LENGTH OF COMMAND
CMDTEXT  DS    CL200         COMMAND ITSELF
SYSTEMLN EQU   *-SYSTMWRK    LENGTH OF DYNAMIC STORAGE
         CSECT ,
*
***********************************************************************
*                                                                     *
*  IDCAMS - dummy function to keep VSE happy                          *
*                                                                     *
***********************************************************************
         ENTRY @@IDCAMS
@@IDCAMS DS    0H
         SAVE  (14,12),,@@IDCAMS
         LR    R12,R15
         USING @@IDCAMS,R12
*
         LA    R15,0
*
         RETURN (14,12),RC=(15)
         LTORG
*
***********************************************************************
*                                                                     *
*  DYNAL - dummy function to keep VSE happy                           *
*                                                                     *
***********************************************************************
         ENTRY @@DYNAL
@@DYNAL  DS    0H
         SAVE  (14,12),,@@DYNAL
         LR    R12,R15
         USING @@DYNAL,R12
*
         LA    R15,0
*
         RETURN (14,12),RC=(15)
         LTORG
*
*
***********************************************************************
*                                                                     *
*  SVC99 - dummy function to keep VSE happy                           *
*                                                                     *
***********************************************************************
         ENTRY @@SVC99
@@SVC99  DS    0H
         SAVE  (14,12),,@@SVC99
         LR    R12,R15
         USING @@SVC99,R12
*
         LA    R15,0
*
         RETURN (14,12),RC=(15)
         LTORG
*
*
*
* Keep the below functions last because they use different
* base registers
*
         DROP  R12
*
*
*
* This is used by VSEFIL
*
TABDDN   DSECT
         USING     *,R9
DDN      DS        CL8
POINTER  DS        F
TABLEN   EQU       *-TABDDN
*
         CSECT
***********************************************************************
*                                                                     *
*  VSEFIL - contributed by Louis Millon                               *
*                                                                     *
*  Allows access to CIL in order to read RECFM=U binary files in a    *
*  PDS-like manner.                                                   *
*                                                                     *
*  CALL      @@VSEFIL,(OPEN,DDN)                                      *
*  CALL      @@VSEFIL,(GET,DDN,RECADDR,RECLEN)                        *
*  CALL      @@VSEFIL,(CLOSE,DDN)                                     *
*                                                                     *
*  "OPEN" etc must be CL8 with that string. DDN is CL8. Other two F   *
*                                                                     *
***********************************************************************
         ENTRY @@VSEFIL
@@VSEFIL DS    0H
         USING     *,R3
         SAVE      (14,12)
         LR        R3,R15
         LR        R10,R1
         B         DEBCODE
MAXFILE  EQU       200                           NUMBER OF FILES
*                                                WHICH MAY BE OPENED AT
*                                                THE SAME TIME
AREA     DC        (TABLEN*MAXFILE)X'00'
         DC        F'-1'                         END OF TABLE
FILENAME DS        CL8
DEBCODE  DS        0H
         L         R15,0(R10)                    FUNCTION
         CLC       =C'GET',0(R15)
         BE        GET
         CLC       =C'OPEN',0(R15)
         BE        OPEN
         CLC       =C'CLOSE',0(R15)
         BE        CLOSE
         RETURN    (14,12),RC=8                  INVALID FUNCTION
OPEN     DS        0H
         L         R15,4(R10)
         MVC       FILENAME,0(R15)               DDNAME
         LA        R9,AREA
         LA        R15,MAXFILE
LOOPOPEN DS        0H
         CLC       DDN,FILENAME
         BE        ALREADY                       THIS FILE IS ALREADY
*                                                OPENED
         LA        R9,TABLEN(R9)
         BCT       R15,LOOPOPEN                  THE FILE IS NOT OPEN
         LA        R9,AREA                       SEEK FOR A VACANT
         LA        R15,MAXFILE                   POSITION IN THE ARRAY
LOOPOPN2 DS        0H
         CLC       DDN,=8X'0'                    POSITION IS FREE?
         BE        OKOPEN                        YES
         LA        R9,TABLEN(R9)
         BCT       R15,LOOPOPN2                  NEXT OCCURENCE
         RETURN    (14,12),RC=12                 ARRAY IS FULL
ALREADY  RETURN    (14,12),RC=8                  FILE ALREADY OPENED
OKOPEN   DS        0H
         LA        R1,FILENAME
         CDLOAD    (1)
         ST        R0,POINTER
         LTR       R15,R15
         BZ        R15OK
         LNR       R15,R15
         RETURN    (14,12),RC=(15)              CDLOAD FAIL
R15OK    EQU       *
         MVC       DDN,FILENAME
         RETURN    (14,12),RC=0
CLOSE    DS        0H
         L         R15,4(R10)
         MVC       FILENAME,0(R15)
         LA        R9,AREA
         LA        R15,MAXFILE
LOOPCLOS DS        0H
         CLC       DDN,FILENAME
         BE        OKCLOSE
         LA        R9,TABLEN(R9)
         BCT       R15,LOOPCLOS
         RETURN    (14,12),RC=8                  DDNAME NOTFND IN ARRAY
OKCLOSE  DS        0H
         LA        R1,FILENAME
*
* This function is not available on DOS/VS, which is a real
* shame. It should probably be added to VSE/380 (at least as
* a dummy) and then reinstated, so that it produces better
* results on z/VSE.
*         CDDELETE  (1)                       REMOVE PHASE FROM GETV
         XC        DDN,DDN
         XC        POINTER,POINTER
         RETURN    (14,12),RC=0
GET      DS        0H
         LA        R15,FILENAME
         MVC       FILENAME,0(R15)
         LA        R9,AREA
         LA        R15,MAXFILE
LOOPGET  DS        0H
         CLC       DDN,FILENAME
         BE        OKGET
         LA        R9,TABLEN(R9)
         BCT       R15,LOOPGET
         RETURN    (14,12),RC=12                 DDNAME NOTFND IN ARRAY
OKGET    DS        0H
         L         R15,POINTER
         CLC       0(4,R15),=F'0'
         BNE       NOEOF
         RETURN    (14,12),RC=8                  EOF
NOEOF    DS        0H
         L         R14,POINTER
         L         R15,12(R10)
         MVC       0(4,R15),0(R14)               LENGTH OF RECORD
         LA        R14,4(R14)                    SKIP RECLEN
         L         R15,08(R10)
         ST        R14,0(R15)                    AADR OF RECORD
         L         R14,POINTER
         AL        R14,0(R14)                    SKIP RECORD
         LA        R14,4(R14)                    AND LENGTH
         ST        R14,POINTER                   NEXT RECORD
         RETURN    (14,12),RC=0
         LTORG
         DROP      R3
*
***********************************************************************
*                                                                     *
*  GETTZ - Get the offset from GMT in 1.048576 seconds                *
*                                                                     *
***********************************************************************
         ENTRY @@GETTZ
@@GETTZ  LA    R15,0
         BR    R14
*
***********************************************************************
*                                                                     *
*  SETJ - SAVE REGISTERS INTO ENV                                     *
*                                                                     *
***********************************************************************
         ENTRY @@SETJ
         USING @@SETJ,R15
@@SETJ   L     R15,0(R1)          get the env variable
         STM   R0,R14,0(R15)      save registers to be restored
         LA    R15,0              setjmp needs to return 0
         BR    R14                return to caller
         LTORG ,
*
***********************************************************************
*                                                                     *
*  LONGJ - RESTORE REGISTERS FROM ENV                                 *
*                                                                     *
***********************************************************************
         ENTRY @@LONGJ
         USING @@LONGJ,R15
@@LONGJ  L     R2,0(R1)           get the env variable
         L     R15,60(R2)         get the return code
         LM    R0,R14,0(R2)       restore registers
         BR    R14                return to caller
         LTORG ,
*
* S/370 doesn't support switching modes so this code is useless,
* and won't compile anyway because "BSM" is not known.
*
         AIF   ('&ZSYS' EQ 'S370').NOMODE If S/370 we can't switch mode
***********************************************************************
*                                                                     *
*  SETM24 - Set AMODE to 24                                           *
*                                                                     *
***********************************************************************
         ENTRY @@SETM24
         USING @@SETM24,R15
@@SETM24 ICM   R14,8,=X'00'       Sure hope caller is below the line
         BSM   0,R14              Return in amode 24
*
***********************************************************************
*                                                                     *
*  SETM31 - Set AMODE to 31                                           *
*                                                                     *
***********************************************************************
         ENTRY @@SETM31
         USING @@SETM31,R15
@@SETM31 ICM   R14,8,=X'80'       Set to switch mode
         BSM   0,R14              Return in amode 31
         LTORG ,
*
.NOMODE  ANOP  ,                  S/370 doesn't support MODE switching
*
*
*
WORKAREA DSECT
SAVEAREA DS    18F
WORKLEN  EQU   *-WORKAREA
* Note that the handle starts from the WORKAREA DSECT, but
* initialization starts at ZDCBAREA (since we don't want to
* initialize our save area). Some more appropriate names
* should probably be found. And the WORKLEN is for functions
* unrelated to I/O which don't need access to the DCB stuff.
* ZDCBLEN includes the length of the work area, since the
* I/O functions still need a save area.
ZDCBAREA DS    0H
PTRDTF   DS    F                  Pointer to the DTF in use
DCBLRECL DS    F                  Logical record length
DCBRECFM DS    F                  Record format
*
* In the case of read, the internal assembler routines require
* a buffer (below the line) to read into, before the data can
* be given to the C caller.
* In the case of write, the C caller needs a BTL buffer to
* write to.
ASMBUF   DS    A                  Pointer to a 32k area for I/O
MEMBER24 DS    CL8
ISMEM    DS    F                  Flag whether this is a PDS
ISDI     DS    F                  Flag whether this is dev-independent
PMVF     DS    0F
P1VF     DS    A
P2VF     DS    A
P3VF     DS    A
P4VF     DS    A
ZDCBLEN  EQU   *-WORKAREA
*
         END
