/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*  Modifications by Dave Edwards, released to the Public Domain     */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  stdio.c - implementation of stuff in stdio.h                     */
/*                                                                   */
/*  The philosophy of the PC/Unix/ASCII implementation is explained  */
/*  here. For the MVS/CMS/EBCDIC implementation, see halfway down    */
/*  this source file (or search for "design of MVS").                */
/*                                                                   */
/*  There is a static array containing pointers to file objects.     */
/*  This is required in order to close all the files on program      */
/*  termination.                                                     */
/*                                                                   */
/*  In order to give speed absolute priority, so that people don't   */
/*  resort to calling DosRead themselves, there is a special flag    */
/*  in the FILE object called "quickbin".  If this flag is set to 1  */
/*  it means that it is a binary file and there is nothing in the    */
/*  buffer and there are no errors, so don't stuff around, just call */
/*  DosRead.                                                         */
/*                                                                   */
/*  When a buffer exists, which is most of the time, fbuf will point */
/*  to it.  The size of the buffer is given by szfbuf.  upto will    */
/*  point to the next character to be read.  endbuf will point PAST  */
/*  the last valid character in the buffer.  bufStartR represents    */
/*  the position in the file that the first character in the buffer  */
/*  is at.  This is only updated when a new buffer is read in.       */
/*                                                                   */
/*  After file open, for a file being read, bufStartR will actually  */
/*  be a negative number, which if added to the position of upto     */
/*  will get to 0.  On a file being written, bufStartR will be set   */
/*  to 0, and upto will point to the start of the buffer.  The       */
/*  reason for the difference on the read is in order to tell the    */
/*  difference between an empty buffer and a buffer with data in it, */
/*  but which hasn't been used yet.  The alternative would be to     */
/*  either keep track of a flag, or make fopen read in an initial    */
/*  buffer.  But we want to avoid reading in data that no-one has    */
/*  yet requested.                                                   */
/*                                                                   */
/*  The buffer is organized as follows...                            */
/*  What we have is an internal buffer, which is 8 characters        */
/*  longer than the actually used buffer.  E.g. say BUFSIZ is        */
/*  512 bytes, then we actually allocate 520 bytes.  The first       */
/*  2 characters will be junk, the next 2 characters set to NUL,     */
/*  for protection against some backward-compares.  The fourth-last  */
/*  character is set to '\n', to protect against overscan.  The      */
/*  last 3 characters will be junk, to protect against memory        */
/*  violation.  intBuffer is the internal buffer, but everyone       */
/*  refers to fbuf, which is actually set to the &intBuffer[4].      */
/*  Also, szfbuf is the size of the "visible" buffer, not the        */
/*  internal buffer.  The reason for the 2 junk characters at the    */
/*  beginning is to align the buffer on a 4-byte boundary.           */
/*                                                                   */
/*  On MVS/CMS/VSE/MUSIC, bufStartR is done differently. It starts   */
/*  as 0, and fbuf, endbuf and upto are all set to the same          */
/*  location - the start of the buffer. With no more data            */
/*  available, a read is done, and endbuf changes according to the   */
/*  size of the data read. If the application does an fread for      */
/*  say 60 bytes when reading an F80 dataset, then 60 characters     */
/*  will go directly into the user buffer, then the remaining        */
/*  20 bytes will go into the beginning of the internal buffer, so   */
/*  the bufStartR will be adjusted to that fact, and upto and        */
/*  fbuf will both point to the beginning, and endbuf will be an     */
/*  additional 20 bytes ahead.                                       */
/*                                                                   */
/*********************************************************************/

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "stdarg.h"
#include "ctype.h"
#include "errno.h"
#include "float.h"
#include "limits.h"
#include "stddef.h"

/* VSE is similar to MVS at the moment */
#if defined(__VSE__)
#define __MVS__ 1
#endif

/* PDOS/386 and MSDOS use the same interface most of the time */
/* Note that PDOS is for the 32-bit version, since the 16-bit
   version uses the MSDOS version since it is compatible with it */
/* linux is pretty similar too */
#if defined(__PDOS386__) || defined(__gnu_linux__)
#define __MSDOS__
#endif

#if defined(__MSDOS__) && !defined(__gnu_linux__)
#if defined(__WATCOMC__) && !defined(__32BIT__)
#define CTYP __cdecl
#else
#define CTYP
#endif
extern int CTYP __creat(const char *filename, int mode, int *errind);
extern int CTYP __open(const char *filename, int mode, int *errind);
extern int CTYP __read(int handle, void *buf, size_t len, int *errind);
extern int CTYP __write(int handle, const void *buf, size_t len, int *errind);
extern int CTYP __seek(int handle, long offset, int whence);
extern void CTYP __close(int handle);
extern void CTYP __remove(const char *filename);
extern void CTYP __rename(const char *old, const char *newnam);
#endif

#ifdef __OS2__
#include <os2.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#if defined(__MVS__) || defined(__CMS__)
#include "mvssupa.h"
#define FIXED_BINARY 0
#define VARIABLE_BINARY 1
#define FIXED_TEXT 2
#define VARIABLE_TEXT 3
#endif

#if defined(__gnu_linux__)

extern int __open(const char *a, int b, int c);
extern int __write(int a, const void *b, int c);
extern int __read(int a, void *b, int c);

#define O_WRONLY 0x1
#define O_CREAT  0x40
#define O_TRUNC  0x200
#define O_RDONLY 0x0

static int open(const char *a, int b, int *c)
{
    int ret;

    *c = 0;
    if (b)
    {
        ret = __open(a, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    }
    else
    {
        ret = __open(a, O_RDONLY, 0);
    }
    if (ret < 0)
    {
        *c = 1;
    }
    return (ret);
}

#define __open(a,b,c) (open((a),(b),(c)))
#define __write(a,b,c,d) (*(d) = 0, (__write)((a),(b),(c)))
#define __read(a,b,c,d) (*(d) = 0, (__read)((a),(b),(c)))

#endif

static FILE permFiles[3];

#define unused(x) ((void)(x))
#define outch(ch) ((fq == NULL) ? *s++ = (char)ch : putc(ch, fq))
#define inch() ((fp == NULL) ? \
    (ch = (unsigned char)*s++) : (ch = getc(fp)))

/* We need to choose whether we are doing move mode or
   locate mode */
#if !LOCMODE /* move mode */


#if defined(__VSE__)

/* for VSE, library files are actually written to memory
   during processing */

#define lbegwrite(stream, len) \
   ( \
    ((stream)->vselupto + (len) > (stream->vselend)) ? \
     (lenwrite = 0, dptr = NULL) : \
     (lenwrite = (len), dptr = (unsigned char *)(stream)->vselupto) \
   )
#define lfinwrite(stream) (((stream)->vselupto += lenwrite), lenwrite)

#define vbegwrite(stream, len) (lenwrite = (len), dptr = (stream)->asmbuf)
#define vfinwrite(stream) (__awrite((stream)->hfile, &dptr, &lenwrite))

#define begwrite(stream, len) ((stream)->vse_punch ? \
    lbegwrite((stream), (len)) : vbegwrite((stream), (len)))
#define finwrite(stream) ((stream)->vse_punch ? \
    lfinwrite(stream) : vfinwrite(stream))

#else
#define begwrite(stream, len) (lenwrite = (len), dptr = (stream)->asmbuf)
#define finwrite(stream) (__awrite((stream)->hfile, &dptr, &lenwrite))
#endif

#else /* locate mode */
#define begwrite(stream, len) (lenwrite = (len), \
    __awrite((stream)->hfile, &dptr, &lenwrite))
#define finwrite(stream)
#endif

#if defined(__MVS__) || defined(__CMS__)
int __doperm = 0; /* are we doing the permanent datasets? */
extern int __tso; /* are we in a TSO environment? */
static unsigned char *dptr;
static size_t lenwrite;
static int    inseek = 0;
static size_t lenread;
#define __aread(a,b) ((__aread)((a),(b),&lenread))
#endif


FILE *__stdin_ptr = &permFiles[0];
FILE *__stdout_ptr = &permFiles[1];
FILE *__stderr_ptr = &permFiles[2];

FILE *__userFiles[__NFILE];
static FILE  *myfile;
static int    spareSpot;
static int    err;
static int    inreopen = 0;

#if defined(__VSE__)
/* for VSE library files being punched */
#define VSE_LIB_LIM 1000000
static char *__vsepb = NULL;
FILE *__stdpch = NULL;
#endif

static const char *fnm;
static const char *modus;
static int modeType;

__PDPCLIB_API__ FILE **__gtin()
    { return(&__stdin_ptr); }
__PDPCLIB_API__ FILE **__gtout()
    { return(&__stdout_ptr); }
__PDPCLIB_API__ FILE **__gterr()
    { return(&__stderr_ptr); }

#if defined(__WIN32__) && !defined(__STATIC__)
__PDPCLIB_API__ __DUMMYFILE _iob[3];
#endif

static void dblcvt(double num, char cnvtype, size_t nwidth,
                   int nprecision, char *result);
static int vvprintf(const char *format, va_list arg, FILE *fq, char *s);
static int vvscanf(const char *format, va_list arg, FILE *fp, const char *s);
static void fopen2(void);
static void fopen3(void);
static void findSpareSpot(void);
static void checkMode(void);
static void osfopen(void);

#if !defined(__MVS__) && !defined(__CMS__)
static void fwriteSlow(const void *ptr,
                       size_t size,
                       size_t nmemb,
                       FILE *stream,
                       size_t towrite,
                       size_t *elemWritten);
static void fwriteSlowT(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten);
static void fwriteSlowB(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten);
static void freadSlowT(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead);
static void freadSlowB(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead);
#endif

static int examine(const char **formt, FILE *fq, char *s, va_list *arg,
                   int chcount);

#if defined(__CMS__) || defined(__MVS__)
static void filedef(char *fdddname, char *fnm, int mymode);
static void fdclr(char *ddname);
#endif
#ifdef __CMS__
extern void __SVC202 ( char *s202parm, int *code, int *parm );
static int cmsrename(const char *old, const char *newnam);
static int cmsremove(const char *filename);
static char *int_strtok(char *s1, const char *s2);
#define strtok int_strtok
#endif


__PDPCLIB_API__ int printf(const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vfprintf(__stdout, format, arg);
    va_end(arg);
    return (ret);
}

__PDPCLIB_API__ int fprintf(FILE *stream, const char *format, ...)
{
    va_list arg;
    int ret;

    stream = __INTFILE(stream);

    va_start(arg, format);
    ret = vfprintf(stream, format, arg);
    va_end(arg);
    return (ret);
}

__PDPCLIB_API__ int vfprintf(FILE *stream, const char *format, va_list arg)
{
    int ret;

    stream = __INTFILE(stream);

    stream->quickText = 0;
    ret = vvprintf(format, arg, stream, NULL);
    return (ret);
}

__PDPCLIB_API__ FILE *fopen(const char *filename, const char *mode)
{
#if defined(__VSE__)
    char *p;
    char *q;
    char memname[9];
    int memlen;
    char phase[80];

    /* for VSE, we cannot write directly to a library, we
       instead need to punch appropriate controls */
    /* note that both w and wb are treated the same */
    if ((*mode == 'w') && ((p = strchr(filename, '(')) != NULL))
    {
        q = strchr(filename, ')');
        if (q <= p) return (NULL);
        memlen = q - p - 1;
        if (memlen > (sizeof memname - 1))
        {
            memlen = (sizeof memname - 1);
        }
        memcpy(memname, p + 1, memlen);
        memname[memlen] = '\0';
        for (p = memname; *p != '\0'; p++)
        {
            *p = toupper((unsigned char)*p);
        }
        if (__stdpch == NULL)
        {
            __vsepb = malloc(VSE_LIB_LIM + 4);
            if (__vsepb == NULL) return (NULL);
            __stdpch = fopen("dd:syspunch", "wb");
            if (__stdpch != NULL)
            {
                __stdpch->vse_punch = 2;
                __stdpch->vselupto = __vsepb;
                __stdpch->vselend = __vsepb + VSE_LIB_LIM;
                __stdpch->reallyu = 1;
            }
        }
        if (__stdpch != NULL)
        {
            __stdpch->vse_punch = 0;
            memset(phase, ' ', sizeof phase);
            sprintf(phase, " PHASE %s,*", memname);
            phase[strlen(phase)] = ' ';
            fwrite(phase, 1, sizeof phase, __stdpch);
            __stdpch->vse_punch = 2; /* in an active file */
        }
        return (__stdpch);
    }
#endif

    fnm = filename;
    modus = mode;
    err = 0;
    if (!inreopen)
    {
        myfile = malloc(sizeof(FILE));
    }
    if (myfile == NULL)
    {
        err = 1;
    }
    else
    {
        if (!inreopen)
        {
            myfile->permfile = 0;
        }
#if defined(__MVS__) || defined(__CMS__)
        if (__doperm)
        {
            myfile->permfile = 1;
        }
#endif
        if (inreopen)
        {
            spareSpot = myfile->intFno;
        }
        else if (!myfile->permfile)
        {
            findSpareSpot();
        }
        if (!err)
        {
            fopen2();
        }
        if (err && !inreopen)
        {
            free(myfile);
        }
    }
    if (err && !inreopen)
    {
        myfile = NULL;
    }
#if defined(__VSE__)
    else
    {
        myfile->vse_punch = 0;
    }
#endif
    return (myfile);
}

static void fopen2(void)
{
    checkMode();
    if (!err)
    {
        strcpy(myfile->modeStr, modus);
        osfopen();
        if (!err)
        {
            if (myfile->permfile)
            {
                myfile->intFno = 0;
            }
            else
            {
                __userFiles[spareSpot] = myfile;
                myfile->intFno = spareSpot;
            }
            fopen3();
        }
    }
    return;
}

static void fopen3(void)
{
    myfile->intBuffer = malloc(BUFSIZ + 8);
    if (myfile->intBuffer == NULL)
    {
        err = 1;
    }
    else
    {
        myfile->theirBuffer = 0;
        myfile->fbuf = myfile->intBuffer + 2;
        *myfile->fbuf++ = '\0';
        *myfile->fbuf++ = '\0';
        myfile->szfbuf = BUFSIZ;
#if !defined(__MVS__) && !defined(__CMS__)
        myfile->quickText = 0;
#endif
        myfile->noNl = 0;
        myfile->endbuf = myfile->fbuf + myfile->szfbuf;
        *myfile->endbuf = '\n';
#if defined(__MVS__) || defined(__CMS__)
        myfile->upto = myfile->fbuf;
        myfile->szfbuf = myfile->lrecl;
        myfile->endbuf = myfile->fbuf; /* for read only */
#else
        myfile->upto = myfile->endbuf;
#endif
#if defined(__MVS__) || defined(__CMS__)
        myfile->bufStartR = 0;
#else
        myfile->bufStartR = -(long)myfile->szfbuf;
#endif
        myfile->justseeked = 0;
        if (myfile->permfile)
        {
            myfile->bufTech = _IOLBF;
        }
        else
        {
            myfile->bufTech = _IOFBF;
        }
        myfile->errorInd = 0;
        myfile->eofInd = 0;
        myfile->ungetCh = -1;
        myfile->update = 0;
        myfile->isopen = 1;
#if !defined(__MVS__) && !defined(__CMS__)
        if (!myfile->textMode)
        {
            myfile->quickBin = 1;
        }
        else
        {
            myfile->quickBin = 0;
        }
#endif
        myfile->mode = __READ_MODE;
        switch (modeType)
        {
            case 2:
            case 3:
            case 5:
            case 6:
            case 8:
            case 9:
            case 11:
            case 12:
                myfile->bufStartR = 0;
                myfile->upto = myfile->fbuf;
                myfile->mode = __WRITE_MODE;
#if defined(__MVS__) || defined(__CMS__)
                myfile->endbuf = myfile->fbuf + myfile->szfbuf;
#endif
                break;
        }
        switch (modeType)
        {
            case 7:
            case 8:
            case 10:
            case 11:
            case 12:
                myfile->update = 1;
                break;
        }
    }
    return;
}

static void findSpareSpot(void)
{
    int x;

    for (x = 0; x < __NFILE; x++)
    {
        if (__userFiles[x] == NULL)
        {
            break;
        }
    }
    if (x == __NFILE)
    {
        err = 1;
    }
    else
    {
        spareSpot = x;
    }
    return;
}

/* checkMode - interpret mode string */
/* r = 1 */
/* w = 2 */
/* a = 3 */
/* rb = 4 */
/* wb = 5 */
/* ab = 6 */
/* r+ = 7 */
/* w+ = 8 */
/* a+ = 9 */
/* r+b or rb+ = 10 */
/* w+b or wb+ = 11 */
/* a+b or ab+ = 12 */

static void checkMode(void)
{
    if (strncmp(modus, "r+b", 3) == 0)
    {
        modeType = 10;
    }
    else if (strncmp(modus, "rb+", 3) == 0)
    {
        modeType = 10;
    }
    else if (strncmp(modus, "w+b", 3) == 0)
    {
        modeType = 11;
    }
    else if (strncmp(modus, "wb+", 3) == 0)
    {
        modeType = 11;
    }
    else if (strncmp(modus, "a+b", 3) == 0)
    {
        modeType = 12;
    }
    else if (strncmp(modus, "ab+", 3) == 0)
    {
        modeType = 12;
    }
    else if (strncmp(modus, "r+", 2) == 0)
    {
        modeType = 7;
    }
    else if (strncmp(modus, "w+", 2) == 0)
    {
        modeType = 8;
    }
    else if (strncmp(modus, "a+", 2) == 0)
    {
        modeType = 9;
    }
    else if (strncmp(modus, "rb", 2) == 0)
    {
        modeType = 4;
    }
    else if (strncmp(modus, "wb", 2) == 0)
    {
        modeType = 5;
    }
    else if (strncmp(modus, "ab", 2) == 0)
    {
        modeType = 6;
    }
    else if (strncmp(modus, "r", 1) == 0)
    {
        modeType = 1;
    }
    else if (strncmp(modus, "w", 1) == 0)
    {
        modeType = 2;
    }
    else if (strncmp(modus, "a", 1) == 0)
    {
        modeType = 3;
    }
    else
    {
        err = 1;
        return;
    }
    if ((modeType == 4)
        || (modeType == 5)
        || (modeType == 6)
        || (modeType == 10)
        || (modeType == 11)
        || (modeType == 12))
    {
        myfile->textMode = 0;
    }
    else
    {
        myfile->textMode = 1;
    }
    return;
}

static void osfopen(void)
{
#ifdef __OS2__
    APIRET rc;
    ULONG  action;
    ULONG  newsize = 0;
    ULONG  fileAttr = 0;
    ULONG  openAction = 0;
    ULONG  openMode = 0;

    if ((modeType == 1) || (modeType == 4) || (modeType == 7)
        || (modeType == 10))
    {
        openAction |= OPEN_ACTION_FAIL_IF_NEW;
        openAction |= OPEN_ACTION_OPEN_IF_EXISTS;
    }
    else if ((modeType == 2) || (modeType == 5) || (modeType == 8)
             || (modeType == 11))
    {
        openAction |= OPEN_ACTION_CREATE_IF_NEW;
        openAction |= OPEN_ACTION_REPLACE_IF_EXISTS;
    }
    else if ((modeType == 3) || (modeType == 6) || (modeType == 9)
             || (modeType == 12))
    {
        openAction |= OPEN_ACTION_CREATE_IF_NEW;
        openAction |= OPEN_ACTION_OPEN_IF_EXISTS;
    }
    openMode |= OPEN_SHARE_DENYWRITE;
    if ((modeType == 1) || (modeType == 4))
    {
        openMode |= OPEN_ACCESS_READONLY;
    }
    else if ((modeType == 2) || (modeType == 3) || (modeType == 5)
             || (modeType == 6))
    {
        openMode |= OPEN_ACCESS_WRITEONLY;
    }
    else
    {
        openMode |= OPEN_ACCESS_READWRITE;
    }
    if ((strlen(fnm) == 2)
        && (fnm[1] == ':')
        && (openMode == OPEN_ACCESS_READONLY))
    {
        openMode |= OPEN_FLAGS_DASD;
    }
    rc = DosOpen((PSZ)fnm,
                 &myfile->hfile,
                 &action,
                 newsize,
                 fileAttr,
                 openAction,
                 openMode,
                 NULL);
    if (rc != 0)
    {
        err = 1;
        errno = rc;
    }
#endif
#ifdef __WIN32__
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = FILE_SHARE_READ;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

    if ((modeType == 1) || (modeType == 4) || (modeType == 7)
        || (modeType == 10))
    {
        dwCreationDisposition = OPEN_EXISTING;
    }
    else if ((modeType == 2) || (modeType == 5) || (modeType == 8)
             || (modeType == 11))
    {
        dwCreationDisposition = CREATE_ALWAYS;
    }
    else if ((modeType == 3) || (modeType == 6) || (modeType == 9)
             || (modeType == 12))
    {
        dwCreationDisposition = CREATE_ALWAYS;
    }
    if ((modeType == 1) || (modeType == 4))
    {
        dwDesiredAccess = GENERIC_READ;
    }
    else if ((modeType == 2) || (modeType == 3) || (modeType == 5)
             || (modeType == 6))
    {
        dwDesiredAccess = GENERIC_WRITE;
    }
    else
    {
        dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    }
    myfile->hfile = CreateFile(fnm,
                               dwDesiredAccess,
                               dwShareMode,
                               NULL,
                               dwCreationDisposition,
                               dwFlagsAndAttributes,
                               NULL);
    if (myfile->hfile == INVALID_HANDLE_VALUE)
    {
        err = 1;
        errno = GetLastError();
    }
#endif
#ifdef __MSDOS__
    int mode;
    int errind;

    if ((modeType == 1) || (modeType == 4)
        || (modeType == 7) || (modeType == 10))
    {
        mode = 0; /* read */
    }
    else if ((modeType == 2) || (modeType == 5)
             || (modeType == 8) || (modeType == 11))
    {
        mode = 1; /* write */
    }
    else
    {
        mode = 2; /* append or otherwise unsupported */
        /* because we don't have append mode implemented
           at the moment on MSDOS, just return with an
           error immediately */
        err = 1;
        errno = 2;
        return;
    }
    if (mode)
    {
#ifdef __gnu_linux__
        myfile->hfile = __open(fnm, 1, &errind);
#else
        myfile->hfile = __creat(fnm, 0, &errind);
#endif
    }
    else
    {
        myfile->hfile = __open(fnm, 0, &errind);
    }
    if (errind)
    {
        err = 1;
        errno = myfile->hfile;
    }
#endif
#if defined(__MVS__) || defined(__CMS__)
    int mode;
    char *p;
    int len;
    char newfnm[FILENAME_MAX];
    char tmpdd[9];

    if ((modeType == 1) || (modeType == 4))
    {
        mode = 0;
    }
    else if ((modeType == 2) || (modeType == 5))
    {
        mode = 1;
    }
    else
    {
        mode = 2;

        /* because we don't have the ability to update files
           at the moment on MVS or CMS, just return with an
           error immediately */
        err = 1;
        errno = 2;
        return;
    }

    myfile->pdsmem[0] = '\0'; /* seek needs to know if member provided */

    if (!inseek)
    {
        myfile->dynal = 0;
    }
/* dw */
/* This code needs changing for VM */
    p = strchr(fnm, ':');
    if ((p != NULL)
        && ((strncmp(fnm, "dd", 2) == 0)
            || (strncmp(fnm, "DD", 2) == 0)))
    {
        p++;
    }
    else
/* if we are in here then there is no "dd:" on front of file */
/* if its CMS generate a ddname and issue a filedef for the file */
#if defined(__CMS__)
    {
/* create a DD from the handle number */
        strcpy(newfnm, fnm);
        p = newfnm;
        while (*p != '\0')
        {
            *p = toupper((unsigned char)*p);
            p++;
        }
        if (!myfile->permfile)
        {
            sprintf(tmpdd, "PDP%03dHD", spareSpot);
        }
        else
        {
            if (myfile == __stdout)
            {
                strcpy(tmpdd, "PDPOUTHD");
            }
            else if (myfile == __stdin)
            {
                strcpy(tmpdd, "PDPINXHD");
            }
            else if (myfile == __stderr)
            {
                strcpy(tmpdd, "PDPERRHD");
            }
        }
        filedef(tmpdd, newfnm, mode);
        myfile->dynal = 1;
        p = tmpdd;
    }
#elif defined(__MVS__)

#if !defined(MUSIC) /* for MUSIC, send everything through to SVC99 */
    if ((strchr(fnm, '\'') == NULL) && (strchr(fnm, '(') == NULL)
#if defined(__MVS__)
    && !__tso
#endif
    )
#endif
    {
        strcpy(newfnm, fnm);
        p = newfnm;

        /* The SVC 99 interface on MVS requires an uppercase
           filename in order to be found via a catalog search */
        while (*p != '\0')
        {
            *p = toupper((unsigned char)*p);
            p++;
        }
        /* create a DD from the handle number */
        if (!myfile->permfile)
        {
            sprintf(tmpdd, "PDP%03dHD", spareSpot);
        }
        else
        {
            if (myfile == __stdout)
            {
                strcpy(tmpdd, "PDPOUTHD");
            }
            else if (myfile == __stdin)
            {
                strcpy(tmpdd, "PDPINXHD");
            }
            else if (myfile == __stderr)
            {
                strcpy(tmpdd, "PDPERRHD");
            }
        }
        fdclr(tmpdd); /* unconditionally clear */
        filedef(tmpdd, newfnm, mode);
        if (err) return;
        myfile->dynal = 1;
        p = tmpdd;
    }

#if !defined(MUSIC)
    /* This is our traditional function for MVS. Keep it for now,
       for the complex strings. For the simple strings, which
       are always used on environments such as PDOS and MUSIC,
       use the code above instead. */
    else
    {
        char rawf[FILENAME_MAX]; /* file name without member,
                                    suitable for dynamic allocation */

        if (!myfile->permfile)
        {
            sprintf(newfnm, "PDP%03dHD", spareSpot);
        }
        else
        {
            if (myfile == __stdout)
            {
                strcpy(newfnm, "PDPOUTHD");
            }
            else if (myfile == __stdin)
            {
                strcpy(newfnm, "PDPINXHD");
            }
            else if (myfile == __stderr)
            {
                strcpy(newfnm, "PDPERRHD");
            }
        }
        strcpy(tmpdd, newfnm);

        strcpy(rawf, "");
        /* strip any single quote */
        if (fnm[0] == '\'')
        {
            fnm++;
        }
#if defined(__MVS__) && !defined(__VSE__)
        else
        {
            /* Currently we manually add a prefix, but in the
               future we should call IKJPARS instead, so that
               an operating system can provide a longer
               (than 7 characters) prefix instead. */
            if (__tso)
            {
                char pfx[8];
                int l;
                char *gp;

                gp = __getpfx();
                if (gp != NULL)
                {
                    memcpy(pfx, gp, sizeof pfx);
                    l = pfx[sizeof pfx - 1];
                    if ((l > 0) && (l < sizeof pfx))
                    {
                        pfx[l] = '\0';
                        strcpy(rawf, pfx);
                        strcat(rawf, ".");
                    }
                }
            }
        }
#endif
        strcat(rawf, fnm);

        /* If we have a file such as "'FRED.C(MARY)'" we need to
           convert this into PDP001HD(MARY) and do a dynamic
           allocation of PDP001HD to "FRED.C". This involves
           extracting the member name and then eliminating the member
           name and any single quotes */
        p = strchr(rawf, '(');
        if (p != NULL)
        {
            *p = '\0';
            p++;
            strcat(newfnm, "(");
            strcat(newfnm, p);

            p = strchr(newfnm, ')');
            if (p != NULL)
            {
                *(p + 1) = '\0';
            }
        }
        else
        {
            /* strip any single quote */
            p = strchr(rawf, '\'');
            if (p != NULL)
            {
                *p = '\0';
            }
        }

        /* MVS requires uppercase filenames */
        p = rawf;
        while (*p != '\0')
        {
            *p = toupper((unsigned char)*p);
            p++;
        }

        /* dynamically allocate file */
        errno = __dynal(strlen(tmpdd), tmpdd, strlen(rawf), rawf);
        if (errno != 0)
        {
            err = 1;
            return;
        }
        myfile->dynal = 1;

        p = newfnm;
    }
#endif /* MUSIC */

#else
    {
        p = (char *)fnm;
    }
#endif
    strcpy(myfile->ddname, "        ");
    len = strcspn(p, "(");
    if (len > 8)
    {
        len = 8;
    }
    memcpy(myfile->ddname, p, len);
    p = myfile->ddname;
    while (*p != '\0')
    {
        *p = toupper((unsigned char)*p);
        p++;
    }

    p = strchr(fnm, '(');
    if (p != NULL)
    {
        p++;
        strcpy(myfile->pdsmem, "        ");
        len = strcspn(p, ")");
        if (len > 8)
        {
            len = 8;
        }
        memcpy(myfile->pdsmem, p, len);
        p = myfile->pdsmem;
        while (*p != '\0')
        {
            *p = toupper((unsigned char)*p);
            p++;
        }
        p = myfile->pdsmem;
    }
    myfile->reallyu = 0;
    myfile->reallyt = 0;
    myfile->asmbuf = 0;

    /* Set some default DCB info. Stress - this will not interfere
       in any way with DCB information the user provides in JCL,
       or on an existing dataset. It is only used when all else
       fails */
    if (myfile->textMode)
    {
        myfile->recfm = __RECFM_V;
        myfile->lrecl = 255;
        if (myfile->permfile)
        {
            /* don't block __stdout/__stderr so that output is not
               delayed if the program crashes */
            myfile->blksize = 259;
        }
        else
        {
            myfile->blksize = 6233;
        }
    }
    else
    {
        myfile->recfm = __RECFM_U;
        myfile->lrecl = 0;
        myfile->blksize = 6233;
    }
#if defined(__MVS__)
    /* If we are dealing with SYSIN/SYSPRINT/SYSTERM and we are
       in a TSO environment, then we should use GETLINE/PUTLINE
       by default, as you would expect for any other TSO
       command, like LISTCAT. If people don't want that, they
       should do a "CALL" to invoke the program as a
       non-TSO-command-processor */
    if (__tso && myfile->permfile)
    {
        mode |= 0x80; /* use PUTLINE/GETLINE if available */
    }
#endif

    myfile->hfile =
        __aopen(myfile->ddname, &mode, &myfile->recfm, &myfile->lrecl,
                &myfile->blksize, &myfile->asmbuf, p);

    /* The true RECFM is not the "recfm" variable. True
       RECFM is as follows:

       x'C0' U, x'80' F, x'40' V
       x'20' T with F, V, or U;
           with top two bits off, x'20' is D (ANSI variable)
           T is obsolete except MVS 3.8 - track overflow
       x'10' B
       x'08' S (standard with F; spanned with V)
       x'04' A
       x'02' M  (never both A and M)
    */

    myfile->true_recfm = (mode >> 16) & 0xff;

    mode &= 0x03; /* only interested in the simple mode now */

    /* errors from MVS __aopen are negative numbers */
    if ((int)myfile->hfile <= 0)
    {
        err = 1;
        errno = -(int)myfile->hfile;
        return;
    }

    /* recfm=f and v are effecively always line-buffered
       because we currently do not support blocking. as
       such, there is no need to switch this flag on at
       the moment, as it is only relevant to recfm=u */
    myfile->line_buf = 0;
    /* if we have a RECFM=U, do special processing */
    if (myfile->recfm == __RECFM_U)
    {
        /* sysprint etc are expected to be line-buffered,
           although we allow full buffering for RECFM=UB */
        if (myfile->permfile
            && ((myfile->true_recfm & 0x10) == 0)
           )
        {
            myfile->line_buf = 1;
        }

        myfile->reallyu = 1;
        myfile->quickBin = 0; /* switch off to be on the safe side */

        /* if open for writing, kludge to switch to fixed */
        if (mode == 1)
        {
            myfile->recfm = __RECFM_F;
        }
        /* if open for reading, kludge to switch to variable */
        else if (mode == 0)
        {
            myfile->recfm = __RECFM_V;
        }
        /* we need to work with a decent lrecl in case the
           assembler routine set the real thing */
        if (myfile->lrecl == 0)
        {
            myfile->lrecl = myfile->blksize;
            if (myfile->lrecl == 0)
            {
                __aclose(myfile);
                err = 1;
                errno = 1;
                return;
            }
        }
    }
    /* if we have RECFM=V, the usable lrecl is 4 bytes shorter
       than we are told, so just adjust that here */
    else if (myfile->recfm == __RECFM_V)
    {
       if (myfile->lrecl > 4)
       {
           myfile->lrecl -= 4;
       }
    }

    if ((modeType == 4) || (modeType == 5))
    {
        myfile->style = 0; /* binary */
    }
    else
    {
        myfile->style = 2; /* text */
        /* for RECFM=U we use binary mode when reading or writing
           text files as we don't want any translation done. But
           record the fact that it was really text mode */
        if (myfile->reallyu)
        {
            myfile->reallyt = 1;
            myfile->style = 0;
        }
    }

    /* by the time we reach here, there is no RECFM=U, so
       we only have 2 forms of binary (starting at 0) and
       two forms of text (starting at 2), so we just need
       to add the recfm (0 or 1) to the above. It should
       probably be done in a less complicated manner! */
    myfile->style += myfile->recfm;

    if (myfile->style == VARIABLE_TEXT)
    {
        myfile->quickText = 1;
    }
    else
    {
        myfile->quickText = 0;
    }
    if (myfile->style == FIXED_BINARY)
    {
        myfile->quickBin = 1;
    }
    else
    {
        myfile->quickBin = 0;
    }
#endif
    return;
}


#if defined(__VSE__)

#define CARDLEN 80
#define MAXCDATA 56  /* maximum data bytes on TXT record */
#define MAXRLEN (MAXCDATA * 10 - 4) /* maximum length of a single record */
    /* the 4 is to ensure the length is never on a card by itself */

static int vseCloseLib(FILE *stream)
{
    char card[CARDLEN];
    size_t cnt;
    size_t tot;
    size_t rem;
    size_t upto;
    size_t x;
    size_t r;
    size_t subtot;

    stream->vse_punch = 0;

    tot = stream->vselupto - __vsepb;
    /* The file needs an EOF marker */
    memset(__vsepb + tot, 0x00, 4);
    tot += 4;

    memset(card, ' ', sizeof card);
    memcpy(card, "\x02" "ESD", 4);
    *(short *)(card + 10) = 0x20; /* length of this ESD is the minimal 0x20 */
    *(short *)(card + 14) = 1; /* CSECT 1 */
    memset(card + 16, ' ', 8); /* name is blank */

    *(int *)(card + 24) = 0; /* assembled origin = 0 */
    *(card + 24) = 0x04; /* PC for some reason */
    *(int *)(card + 28) = 0; /* AMODE + length - for some reason we
        don't need to set the length properly. */

#if 0
    /* is this required? */
    *(int *)(card + 28) = tot +
        (tot/MAXRLEN + ((tot % MAXRLEN) != 0)) * sizeof(int);
#endif

    memcpy(card + 32, "TOTO    ", 8); /* total? */

    /* is this required? */
    *(int *)(card + 44) = tot +
        (tot/MAXRLEN + ((tot % MAXRLEN) != 0)) * sizeof(int);
    fwrite(card, 1, sizeof card, stream);

    subtot = 0;
    for (upto = 0; upto < tot; upto += rem)
    {
        rem = tot - upto;
        if (rem > MAXRLEN)
        {
            rem = MAXRLEN;
        }
        for (x = 0; x < rem; x += r)
        {
            r = rem - x;
            if (r > MAXCDATA)
            {
                r = MAXCDATA;
            }
            if ((x == 0) && (r > (MAXCDATA - sizeof(int))))
            {
                r -= sizeof(int);
            }
            memset(card, ' ', sizeof card);
            memcpy(card, "\x02" "TXT", 4);
            *(int *)(card + 4) = subtot; /* origin */
            card[4] = ' ';
            *(short *)(card + 10) = r + ((x == 0) ? sizeof(int) : 0);
                /* byte count */
            *(int *)(card + 12) = 1; /* CSECT 1 */
            if (x == 0)
            {
                *(int *)(card + 16) = rem;
                if ((upto + rem) >= tot)
                {
                    *(int *)(card + 16) -= 4;
                }
                memcpy(card + 16 + sizeof(int), __vsepb + upto, r);
                subtot += (r + sizeof(int));
            }
            else
            {
                memcpy(card + 16, __vsepb + upto + x, r);
                subtot += r;
            }
            fwrite(card, 1, sizeof card, stream);
        }
    }
    memset(card, ' ', sizeof card);
    memcpy(card, "\x02" "END", 4);
#if 0
    /* is this required? */
    *(int *)(card + 24) = tot +
        (tot/MAXRLEN + ((tot % MAXRLEN) != 0)) * sizeof(int);
#endif
    fwrite(card, 1, sizeof card, stream);

    stream->vselupto = __vsepb;
    stream->vse_punch = 1; /* still the punch, but not active */

    return (0);
}
#endif


__PDPCLIB_API__ int fclose(FILE *stream)
{
#ifdef __OS2__
    APIRET rc;
#endif
#ifdef __WIN32__
    BOOL rc;
#endif

    stream = __INTFILE(stream);

    if (!stream->isopen)
    {
        return (EOF);
    }
    fflush(stream);
#ifdef __VSE__
    /* only take action if in an active file */
    if (stream->vse_punch == 2)
    {
        stream->upto = stream->fbuf;
        stream->bufStartR = 0;
        return (vseCloseLib(stream));
    }
    /* closing an inactive punch must be the real thing, so free
       the buffer and go through the rest of the close logic. */
    else if (stream->vse_punch == 1)
    {
        free(__vsepb);
        __vsepb = NULL;
    }
#endif
#ifdef __OS2__
    rc = DosClose(stream->hfile);
#endif
#ifdef __WIN32__
    rc = CloseHandle(stream->hfile);
#endif
#ifdef __MSDOS__
    __close(stream->hfile);
#endif
#if defined(__MVS__) || defined(__CMS__)
    if ((stream->mode == __WRITE_MODE) && (stream->upto != stream->fbuf))
    {
        if (stream->reallyu)
        {
            /* we should not get to here, because the flush would
               have taken care of it. perhaps we can generate an
               internal error */
        }
        else if (stream->textMode)
        {
            putc('\n', stream);
        }
        else
        {
            size_t remain;
            size_t x;

            remain = stream->endbuf - stream->upto;
            for (x = 0; x < remain; x++)
            {
                putc(0x00, stream);
            }
        }
    }
    __aclose(stream->hfile);
#ifdef __CMS__
    if (stream->dynal && !inseek)
    {
        fdclr(stream->ddname);
    }
#endif
#endif
    if (!stream->theirBuffer)
    {
#if !defined(__MVS__) && !defined(__CMS__) && !defined(__VSE__)
        /* on the PC, permanent files have a static buffer */
        if (!stream->permfile)
#endif
        free(stream->intBuffer);
    }
    if (!stream->permfile && !inreopen)
    {
        __userFiles[stream->intFno] = NULL;
        free(stream);
    }
    else
    {
#if defined(__MVS__) || defined(__CMS__)
        /* if we're not in the middle of freopen ... */
        if (!stream->permfile)
        {
            __userFiles[stream->intFno] = NULL;
        }
        if (!inreopen)
        {
            free(stream);
            /* need to protect against the app closing the file
               which it is allowed to */
            if (stream == __stdin)
            {
                __stdin = NULL;
            }
            else if (stream == __stdout)
            {
                __stdout = NULL;
            }
            else if (stream == __stderr)
            {
                __stderr = NULL;
            }
        }
#else
        stream->isopen = 0;
#endif
    }
#ifdef __OS2__
    if (rc != 0)
    {
        errno = rc;
        return (EOF);
    }
#endif
#ifdef __WIN32__
    if (!rc)
    {
        errno = GetLastError();
        return (EOF);
    }
#endif
    return (0);
}

#if !defined(__MVS__) && !defined(__CMS__)
__PDPCLIB_API__ size_t fread(void *ptr,
                             size_t size,
                             size_t nmemb,
                             FILE *stream)
{
    size_t toread;
    size_t elemRead;
    size_t actualRead;
#ifdef __OS2__
    APIRET rc;
    ULONG tempRead;
#endif
#ifdef __WIN32__
    BOOL rc;
    DWORD tempRead;
#endif
#ifdef __MSDOS__
    int errind;
    size_t tempRead;
#endif

    stream = __INTFILE(stream);

    if (nmemb == 1)
    {
        toread = size;
    }
    else if (size == 1)
    {
        toread = nmemb;
    }
    else
    {
        toread = size * nmemb;
    }
    if (toread < stream->szfbuf)
    {
        stream->quickBin = 0;
    }
    if (stream->ungetCh != -1)
    {
        *--stream->upto = (char)stream->ungetCh;
        stream->ungetCh = -1;
    }
    if (!stream->quickBin)
    {
        /* if we were previously writing and then
           we seeked, and now we're reading, we need to
           correct things */
        if (stream->justseeked)
        {
            stream->justseeked = 0;
            if (stream->mode == __WRITE_MODE)
            {
                stream->bufStartR -= (stream->endbuf - stream->fbuf);
                stream->upto = stream->endbuf;
                stream->mode = __READ_MODE;
            }
        }
        if (stream->textMode)
        {
            freadSlowT(ptr, stream, toread, &actualRead);
        }
        else
        {
            if (toread <= (stream->endbuf - stream->upto))
            {
                memcpy(ptr, stream->upto, toread);
                actualRead = toread;
                stream->upto += toread;
            }
            else
            {
                freadSlowB(ptr, stream, toread, &actualRead);
            }
        }
        if (nmemb == 1)
        {
            if (actualRead == size)
            {
                elemRead = 1;
            }
            else
            {
                elemRead = 0;
            }
        }
        else if (size == 1)
        {
            elemRead = actualRead;
        }
        else
        {
            if (size == 0)
            {
                elemRead = 0;
            }
            else
            {
                elemRead = actualRead / size;
            }
        }
        return (elemRead);
    }
    else
    {
#ifdef __OS2__
        rc = DosRead(stream->hfile, ptr, toread, &tempRead);
        if (rc != 0)
        {
            actualRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
        else
        {
            actualRead = tempRead;
        }
#endif
#ifdef __WIN32__
        rc = ReadFile(stream->hfile,
                      ptr,
                      toread,
                      &tempRead,
                      NULL);
        if (!rc)
        {
            actualRead = 0;
            stream->errorInd = 1;
            errno = GetLastError();
        }
        else
        {
            actualRead = tempRead;
        }
#endif
#ifdef __MSDOS__
        tempRead = __read(stream->hfile, ptr, toread, &errind);
        if (errind)
        {
            errno = tempRead;
            actualRead = 0;
            stream->errorInd = 1;
        }
        else
        {
            actualRead = tempRead;
        }
#endif
        if (nmemb == 1)
        {
            if (actualRead == size)
            {
                elemRead = 1;
            }
            else
            {
                elemRead = 0;
                stream->eofInd = 1;
            }
        }
        else if (size == 1)
        {
            elemRead = actualRead;
            if (nmemb != actualRead)
            {
                stream->eofInd = 1;
            }
        }
        else
        {
            if (size == 0)
            {
                elemRead = 0;
            }
            else
            {
                elemRead = actualRead / size;
            }
            if (toread != actualRead)
            {
                stream->eofInd = 1;
            }
        }
        stream->bufStartR += actualRead;
        return (elemRead);
    }
}


/*
while toread has not been satisfied
{
    scan stuff out of buffer, replenishing buffer as required
}
*/

static void freadSlowT(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead)
{
    int finReading = 0;
    size_t avail;
    size_t need;
    char *p;
    size_t got;
#ifdef __OS2__
    ULONG tempRead;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD tempRead;
    BOOL rc;
#endif
#ifdef __MSDOS__
    size_t tempRead;
    int errind;
#endif

    *actualRead = 0;
    while (!finReading)
    {
        if (stream->upto == stream->endbuf)
        {
#ifdef __OS2__
            rc = DosRead(stream->hfile,
                         stream->fbuf,
                         stream->szfbuf,
                         &tempRead);
            if (rc != 0)
            {
                tempRead = 0;
                stream->errorInd = 1;
                errno = rc;
            }
#endif
#ifdef __WIN32__
            rc = ReadFile(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &tempRead,
                          NULL);
            if (!rc)
            {
                tempRead = 0;
                stream->errorInd = 1;
                errno = GetLastError();
            }
#endif
#ifdef __MSDOS__
            tempRead = __read(stream->hfile,
                              stream->fbuf,
                              stream->szfbuf,
                              &errind);
            if (errind)
            {
                errno = tempRead;
                tempRead = 0;
                stream->errorInd = 1;
            }
#endif
            if (tempRead == 0)
            {
                stream->eofInd = 1;
                break;
            }
            stream->bufStartR += (stream->upto - stream->fbuf);
            stream->endbuf = stream->fbuf + tempRead;
            *stream->endbuf = '\n';
            stream->upto = stream->fbuf;
        }
        avail = (size_t)(stream->endbuf - stream->upto) + 1;
        need = toread - *actualRead;
        p = memchr(stream->upto, '\n', avail);
        got = (size_t)(p - stream->upto);
        if (need < got)
        {
            memcpy((char *)ptr + *actualRead, stream->upto, need);
            stream->upto += need;
            *actualRead += need;
        }
        else
        {
            memcpy((char *)ptr + *actualRead, stream->upto, got);
            stream->upto += got;
            *actualRead += got;
            if (p != stream->endbuf)
            {
                if (*(stream->upto - 1) == '\r')
                {
                    *((char *)ptr + *actualRead - 1) = '\n';
                    stream->upto++;
                }
                else if (need != got)
                {
                    *((char *)ptr + *actualRead) = '\n';
                    *actualRead += 1;
                    stream->upto++;
                }
            }
            else
            {
                if (*(stream->upto - 1) == '\r')
                {
                    *actualRead -= 1;
                }
            }
        }
        if (*actualRead == toread)
        {
            finReading = 1;
        }
    }
    return;
}

static void freadSlowB(void *ptr,
                       FILE *stream,
                       size_t toread,
                       size_t *actualRead)
{
    size_t avail;
#ifdef __OS2__
    ULONG tempRead;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD tempRead;
    BOOL rc;
#endif
#ifdef __MSDOS__
    size_t tempRead;
    int errind;
#endif

    avail = (size_t)(stream->endbuf - stream->upto);
    memcpy(ptr, stream->upto, avail);
    *actualRead = avail;
    stream->bufStartR += (stream->endbuf - stream->fbuf);
    if (toread >= stream->szfbuf)
    {
        stream->upto = stream->endbuf;
        stream->quickBin = 1;
#ifdef __OS2__
        rc = DosRead(stream->hfile,
                     (char *)ptr + *actualRead,
                     toread - *actualRead,
                     &tempRead);
        if (rc != 0)
        {
            tempRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
#endif
#ifdef __WIN32__
            rc = ReadFile(stream->hfile,
                          (char *)ptr + *actualRead,
                          toread - *actualRead,
                          &tempRead,
                          NULL);
            if (!rc)
            {
                tempRead = 0;
                stream->errorInd = 1;
                errno = GetLastError();
            }
#endif
#ifdef __MSDOS__
        tempRead = __read(stream->hfile,
                          (char *)ptr + *actualRead,
                          toread - *actualRead,
                          &errind);
        if (errind)
        {
            errno = tempRead;
            tempRead = 0;
            stream->errorInd = 1;
        }
#endif
        else if (tempRead != (toread - *actualRead))
        {
            stream->eofInd = 1;
        }
        *actualRead += tempRead;
        stream->bufStartR += tempRead;
    }
    else
    {
        size_t left;

        stream->upto = stream->fbuf;
#ifdef __OS2__
        rc = DosRead(stream->hfile,
                     stream->fbuf,
                     stream->szfbuf,
                     &tempRead);
        left = toread - *actualRead;
        if (rc != 0)
        {
            tempRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
#endif
#ifdef __WIN32__
        rc = ReadFile(stream->hfile,
                      stream->fbuf,
                      stream->szfbuf,
                      &tempRead,
                      NULL);
        left = toread - *actualRead;
        if (!rc)
        {
            tempRead = 0;
            stream->errorInd = 1;
            errno = GetLastError();
        }
#endif
#ifdef __MSDOS__
        tempRead = __read(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &errind);
        left = toread - *actualRead;
        if (errind)
        {
            errno = tempRead;
            tempRead = 0;
            stream->errorInd = 1;
        }
#endif
        else if (tempRead < left)
        {
            stream->eofInd = 1;
        }
        stream->endbuf = stream->fbuf + tempRead;
        *stream->endbuf = '\n';
        avail = (size_t)(stream->endbuf - stream->upto);
        if (avail > left)
        {
            avail = left;
        }
        memcpy((char *)ptr + *actualRead,
               stream->upto,
               avail);
        stream->upto += avail;
        *actualRead += avail;
    }
    return;
}
#endif

#if !defined(__MVS__) && !defined(__CMS__)
__PDPCLIB_API__ size_t fwrite(const void *ptr,
                              size_t size,
                              size_t nmemb,
                              FILE *stream)
{
    size_t towrite;
    size_t elemWritten;
#ifdef __OS2__
    ULONG actualWritten;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD actualWritten;
    BOOL rc;
#endif
#ifdef __MSDOS__
    size_t actualWritten;
    int errind;
#endif

    stream = __INTFILE(stream);

    if (nmemb == 1)
    {
        towrite = size;
    }
    else if (size == 1)
    {
        towrite = nmemb;
    }
    else
    {
        towrite = size * nmemb;
    }
    if (towrite < stream->szfbuf)
    {
        stream->quickBin = 0;
        if ((stream->bufTech == _IONBF) && !stream->textMode)
        {
            stream->quickBin = 1;
        }
    }
    if (!stream->quickBin)
    {
        fwriteSlow(ptr, size, nmemb, stream, towrite, &elemWritten);
        return (elemWritten);
    }
    else
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile, (VOID *)ptr, towrite, &actualWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            actualWritten = 0;
            errno = rc;
        }
#endif
#ifdef __WIN32__
        rc = WriteFile(stream->hfile, ptr, towrite, &actualWritten, NULL);
        if (!rc)
        {
            stream->errorInd = 1;
            actualWritten = 0;
            errno = GetLastError();
        }
#endif
#ifdef __MSDOS__
        actualWritten = __write(stream->hfile,
                                ptr,
                                towrite,
                                &errind);
        if (errind)
        {
            stream->errorInd = 1;
            actualWritten = 0;
            errno = actualWritten;
        }
#endif
        if (nmemb == 1)
        {
            if (actualWritten == size)
            {
                elemWritten = 1;
            }
            else
            {
                elemWritten = 0;
            }
        }
        else if (size == 1)
        {
            elemWritten = actualWritten;
        }
        else
        {
            elemWritten = actualWritten / size;
        }
        stream->bufStartR += actualWritten;
        return (elemWritten);
    }
}

static void fwriteSlow(const void *ptr,
                       size_t size,
                       size_t nmemb,
                       FILE *stream,
                       size_t towrite,
                       size_t *elemWritten)
{
    size_t actualWritten;

    if (stream->justseeked)
    {
        stream->justseeked = 0;
        if (stream->mode == __READ_MODE)
        {
            stream->bufStartR += (stream->endbuf - stream->fbuf);
            stream->upto = stream->fbuf;
            stream->mode = __WRITE_MODE;
        }
    }
    if ((stream->textMode) || (stream->bufTech == _IOLBF))
    {
        fwriteSlowT(ptr, stream, towrite, &actualWritten);
    }
    else
    {
        fwriteSlowB(ptr, stream, towrite, &actualWritten);
    }
    if (nmemb == 1)
    {
        if (actualWritten == size)
        {
            *elemWritten = 1;
        }
        else
        {
            *elemWritten = 0;
        }
    }
    else if (size == 1)
    {
        *elemWritten = actualWritten;
    }
    else
    {
        *elemWritten = actualWritten / size;
    }
    return;
}


/* can still be called on binary files, if the binary file is
   line buffered  */

static void fwriteSlowT(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten)
{
    char *p;
    char *tptr;
    char *oldp;
    size_t diffp;
    size_t rem;
    int fin;
#ifdef __OS2__
    ULONG tempWritten;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD tempWritten;
    BOOL rc;
#endif
#ifdef __MSDOS__
    size_t tempWritten;
    int errind;
#endif

    *actualWritten = 0;
    tptr = (char *)ptr;
    p = tptr;
    oldp = p;
    p = (char *)memchr(oldp, '\n', towrite - (size_t)(oldp - tptr));
    while (p != NULL)
    {
        diffp = (size_t)(p - oldp);
        fin = 0;
        while (!fin)
        {
            rem = (size_t)(stream->endbuf - stream->upto);
            if (diffp < rem)
            {
                memcpy(stream->upto, oldp, diffp);
                stream->upto += diffp;
                *actualWritten += diffp;
                fin = 1;
            }
            else
            {
                memcpy(stream->upto, oldp, rem);
                oldp += rem;
                diffp -= rem;
#ifdef __OS2__
                rc = DosWrite(stream->hfile,
                              stream->fbuf,
                              stream->szfbuf,
                              &tempWritten);
                if (rc != 0)
                {
                    stream->errorInd = 1;
                    errno = rc;
                    return;
                }
#endif
#ifdef __WIN32__
                rc = WriteFile(stream->hfile,
                               stream->fbuf,
                               stream->szfbuf,
                               &tempWritten,
                               NULL);
                if (!rc)
                {
                    stream->errorInd = 1;
                    errno = GetLastError();
                    return;
                }
#endif
#ifdef __MSDOS__
                tempWritten = __write(stream->hfile,
                                      stream->fbuf,
                                      stream->szfbuf,
                                      &errind);
                if (errind)
                {
                    stream->errorInd = 1;
                    return;
                }
#endif
                else
                {
                    *actualWritten += rem;
                    stream->upto = stream->fbuf;
                    stream->bufStartR += tempWritten;
                }
            }
        }
        rem = (size_t)(stream->endbuf - stream->upto);
        if (rem < 3)
        {
#ifdef __OS2__
            rc = DosWrite(stream->hfile,
                          stream->fbuf,
                          (size_t)(stream->upto - stream->fbuf),
                          &tempWritten);
            if (rc != 0)
            {
                stream->errorInd = 1;
                errno = rc;
                return;
            }
#endif
#ifdef __WIN32__
            rc = WriteFile(stream->hfile,
                           stream->fbuf,
                           (size_t)(stream->upto - stream->fbuf),
                           &tempWritten,
                           NULL);
            if (!rc)
            {
                stream->errorInd = 1;
                errno = GetLastError();
                return;
            }
#endif
#ifdef __MSDOS__
            tempWritten = __write(stream->hfile,
                                  stream->fbuf,
                                  (size_t)(stream->upto - stream->fbuf),
                                  &errind);
            if (errind)
            {
                stream->errorInd = 1;
                errno = tempWritten;
                return;
            }
#endif
            stream->upto = stream->fbuf;
            stream->bufStartR += tempWritten;
        }
#ifndef __gnu_linux__
        if (stream->textMode)
        {
            memcpy(stream->upto, "\r\n", 2);
            stream->upto += 2;
        }
        else
#endif
        {
            memcpy(stream->upto, "\n", 1);
            stream->upto += 1;
        }
        *actualWritten += 1;
        oldp = p + 1;
        p = (char *)memchr(oldp, '\n', towrite - (size_t)(oldp - tptr));
    }

    if ((stream->bufTech == _IOLBF)
        && (stream->upto != stream->fbuf)
        && (oldp != tptr))
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                      stream->fbuf,
                      (size_t)(stream->upto - stream->fbuf),
                      &tempWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return;
        }
#endif
#ifdef __WIN32__
        rc = WriteFile(stream->hfile,
                       stream->fbuf,
                       (size_t)(stream->upto - stream->fbuf),
                       &tempWritten,
                       NULL);
        if (!rc)
        {
            stream->errorInd = 1;
            errno = GetLastError();
            return;
        }
#endif
#ifdef __MSDOS__
        tempWritten = __write(stream->hfile,
                              stream->fbuf,
                              (size_t)(stream->upto - stream->fbuf),
                              &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = tempWritten;
            return;
        }
#endif
        stream->upto = stream->fbuf;
        stream->bufStartR += tempWritten;
    }

    diffp = towrite - *actualWritten;
    while (diffp != 0)
    {
        rem = (size_t)(stream->endbuf - stream->upto);
        if (diffp < rem)
        {
            memcpy(stream->upto, oldp, diffp);
            stream->upto += diffp;
            *actualWritten += diffp;
        }
        else
        {
            memcpy(stream->upto, oldp, rem);
#ifdef __OS2__
            rc = DosWrite(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &tempWritten);
            if (rc != 0)
            {
                stream->errorInd = 1;
                errno = rc;
                return;
            }
#endif
#ifdef __WIN32__
            rc = WriteFile(stream->hfile,
                           stream->fbuf,
                           stream->szfbuf,
                           &tempWritten,
                           NULL);
            if (!rc)
            {
                stream->errorInd = 1;
                errno = GetLastError();
                return;
            }
#endif
#ifdef __MSDOS__
            tempWritten = __write(stream->hfile,
                                  stream->fbuf,
                                  stream->szfbuf,
                                  &errind);
            if (errind)
            {
                stream->errorInd = 1;
                errno = tempWritten;
                return;
            }
#endif
            else
            {
                *actualWritten += rem;
                stream->upto = stream->fbuf;
            }
            stream->bufStartR += tempWritten;
            oldp += rem;
        }
        diffp = towrite - *actualWritten;
    }
    if ((stream->bufTech == _IONBF)
        && (stream->upto != stream->fbuf))
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                      stream->fbuf,
                      (size_t)(stream->upto - stream->fbuf),
                      &tempWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return;
        }
#endif
#ifdef __WIN32__
        rc = WriteFile(stream->hfile,
                       stream->fbuf,
                       (size_t)(stream->upto - stream->fbuf),
                       &tempWritten,
                       NULL);
        if (!rc)
        {
            stream->errorInd = 1;
            errno = GetLastError();
            return;
        }
#endif
#ifdef __MSDOS__
        tempWritten = __write(stream->hfile,
                              stream->fbuf,
                              (size_t)(stream->upto - stream->fbuf),
                              &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = tempWritten;
            return;
        }
#endif
        stream->upto = stream->fbuf;
        stream->bufStartR += tempWritten;
    }
    return;
}

/* whilst write requests are smaller than a buffer, we do not turn
   on quickbin */

static void fwriteSlowB(const void *ptr,
                        FILE *stream,
                        size_t towrite,
                        size_t *actualWritten)
{
    size_t spare;
#ifdef __OS2__
    ULONG tempWritten;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD tempWritten;
    BOOL rc;
#endif
#ifdef __MSDOS__
    size_t tempWritten;
    int errind;
#endif

    spare = (size_t)(stream->endbuf - stream->upto);
    if (towrite < spare)
    {
        memcpy(stream->upto, ptr, towrite);
        *actualWritten = towrite;
        stream->upto += towrite;
        return;
    }
    memcpy(stream->upto, ptr, spare);
#ifdef __OS2__
    rc = DosWrite(stream->hfile,
                  stream->fbuf,
                  stream->szfbuf,
                  &tempWritten);
    if (rc != 0)
    {
        stream->errorInd = 1;
        errno = rc;
        return;
    }
#endif
#ifdef __WIN32__
    rc = WriteFile(stream->hfile,
                   stream->fbuf,
                   stream->szfbuf,
                   &tempWritten,
                   NULL);
    if (!rc)
    {
        stream->errorInd = 1;
        errno = GetLastError();
        return;
    }
#endif
#ifdef __MSDOS__
    tempWritten = __write(stream->hfile,
                          stream->fbuf,
                          stream->szfbuf,
                          &errind);
    if (errind)
    {
        stream->errorInd = 1;
        errno = tempWritten;
        return;
    }
#endif
    *actualWritten = spare;
    stream->upto = stream->fbuf;
    stream->bufStartR += tempWritten;
    if (towrite > stream->szfbuf)
    {
        stream->quickBin = 1;
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                      (char *)ptr + *actualWritten,
                      towrite - *actualWritten,
                      &tempWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return;
        }
#endif
#ifdef __WIN32__
        rc = WriteFile(stream->hfile,
                       (char *)ptr + *actualWritten,
                       towrite - *actualWritten,
                       &tempWritten,
                       NULL);
        if (!rc)
        {
            stream->errorInd = 1;
            errno = GetLastError();
            return;
        }
#endif
#ifdef __MSDOS__
        tempWritten = __write(stream->hfile,
                              (char *)ptr + *actualWritten,
                              towrite - *actualWritten,
                              &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = tempWritten;
            return;
        }
#endif
        *actualWritten += tempWritten;
        stream->bufStartR += tempWritten;
    }
    else
    {
        memcpy(stream->fbuf,
               (char *)ptr + *actualWritten,
               towrite - *actualWritten);
        stream->upto += (towrite - *actualWritten);
        *actualWritten = towrite;
    }
    stream->bufStartR += *actualWritten;
    return;
}
#endif

static int vvprintf(const char *format, va_list arg, FILE *fq, char *s)
{
    int fin = 0;
    int vint;
    double vdbl;
    unsigned int uvint;
    const char *vcptr;
    int chcount = 0;
    size_t len;
    char numbuf[50];
    char *nptr;
    int *viptr;

    while (!fin)
    {
        if (*format == '\0')
        {
            fin = 1;
        }
        else if (*format == '%')
        {
            format++;
            if (*format == 'd')
            {
                vint = va_arg(arg, int);
                if (vint < 0)
                {
                    uvint = -vint;
                }
                else
                {
                    uvint = vint;
                }
                nptr = numbuf;
                do
                {
                    *nptr++ = (char)('0' + uvint % 10);
                    uvint /= 10;
                } while (uvint > 0);
                if (vint < 0)
                {
                    *nptr++ = '-';
                }
                do
                {
                    nptr--;
                    outch(*nptr);
                    chcount++;
                } while (nptr != numbuf);
            }
            else if (strchr("eEgGfF", *format) != NULL && *format != 0)
            {
                vdbl = va_arg(arg, double);
                dblcvt(vdbl, *format, 0, 6, numbuf);   /* 'e','f' etc. */
                len = strlen(numbuf);
                if (fq == NULL)
                {
                    memcpy(s, numbuf, len);
                    s += len;
                }
                else
                {
                    fputs(numbuf, fq);
                }
                chcount += len;
            }
            else if (*format == 's')
            {
                vcptr = va_arg(arg, const char *);
                if (vcptr == NULL)
                {
                    vcptr = "(null)";
                }
                if (fq == NULL)
                {
                    len = strlen(vcptr);
                    memcpy(s, vcptr, len);
                    s += len;
                    chcount += len;
                }
                else
                {
                    fputs(vcptr, fq);
                    chcount += strlen(vcptr);
                }
            }
            else if (*format == 'c')
            {
                vint = va_arg(arg, int);
                outch(vint);
                chcount++;
            }
            else if (*format == 'n')
            {
                viptr = va_arg(arg, int *);
                *viptr = chcount;
            }
            else if (*format == '%')
            {
                outch('%');
                chcount++;
            }
            else
            {
                int extraCh;

                extraCh = examine(&format, fq, s, &arg, chcount);
                chcount += extraCh;
                if (s != NULL)
                {
                    s += extraCh;
                }
            }
        }
        else
        {
            outch(*format);
            chcount++;
        }
        format++;
    }
    return (chcount);
}

static int examine(const char **formt, FILE *fq, char *s, va_list *arg,
                   int chcount)
{
    int extraCh = 0;
    int flagMinus = 0;
    int flagPlus = 0;
    int flagSpace = 0;
    int flagHash = 0;
    int flagZero = 0;
    int width = 0;
    int precision = -1;
    int half = 0;
    int lng = 0;
    int specifier = 0;
    int fin;
    long lvalue;
    short int hvalue;
    int ivalue;
    unsigned long ulvalue;
    double vdbl;
    char *svalue;
    char work[50];
    int x;
    int y;
    int rem;
    const char *format;
    int base;
    int fillCh;
    int neg;
    int length;
    size_t slen;

    unused(chcount);
    format = *formt;
    /* processing flags */
    fin = 0;
    while (!fin)
    {
        switch (*format)
        {
            case '-': flagMinus = 1;
                      break;
            case '+': flagPlus = 1;
                      break;
            case ' ': flagSpace = 1;
                      break;
            case '#': flagHash = 1;
                      break;
            case '0': flagZero = 1;
                      break;
            case '*': width = va_arg(*arg, int);
                      if (width < 0)
                      {
                          flagMinus = 1;
                          width = -width;
                      }
                      break;
            default:  fin = 1;
                      break;
        }
        if (!fin)
        {
            format++;
        }
        else
        {
            if (flagSpace && flagPlus)
            {
                flagSpace = 0;
            }
            if (flagMinus)
            {
                flagZero = 0;
            }
        }
    }

    /* processing width */
    if (isdigit((unsigned char)*format))
    {
        while (isdigit((unsigned char)*format))
        {
            width = width * 10 + (*format - '0');
            format++;
        }
    }

    /* processing precision */
    if (*format == '.')
    {
        format++;
        if (*format == '*')
        {
            precision = va_arg(*arg, int);
            format++;
        }
        else
        {
            precision = 0;
            while (isdigit((unsigned char)*format))
            {
                precision = precision * 10 + (*format - '0');
                format++;
            }
        }
    }

    /* processing h/l/L */
    if (*format == 'h')
    {
        /* all environments should promote shorts to ints,
           so we should be able to ignore the 'h' specifier.
           It will create problems otherwise. */
        /* half = 1; */
    }
    else if (*format == 'l')
    {
        lng = 1;
    }
    else if (*format == 'L')
    {
        lng = 1;
    }
    else
    {
        format--;
    }
    format++;

    /* processing specifier */
    specifier = *format;

    if (strchr("dxXuiop", specifier) != NULL && specifier != 0)
    {
        if (precision < 0)
        {
            precision = 1;
        }
#if defined(__MSDOS__) && \
    !defined(__PDOS386__) && \
    !defined(__gnu_linux__)
        if (specifier == 'p')
        {
            lng = 1;
        }
#endif
        if (lng)
        {
            lvalue = va_arg(*arg, long);
        }
        else if (half)
        {
            /* short is promoted to int, so use int */
            hvalue = va_arg(*arg, int);
            if (specifier == 'u') lvalue = (unsigned short)hvalue;
            else lvalue = hvalue;
        }
        else
        {
            ivalue = va_arg(*arg, int);
            if (specifier == 'u') lvalue = (unsigned int)ivalue;
            else lvalue = ivalue;
        }
        ulvalue = (unsigned long)lvalue;
        if ((lvalue < 0) && ((specifier == 'd') || (specifier == 'i')))
        {
            neg = 1;
            ulvalue = -lvalue;
        }
        else
        {
            neg = 0;
        }
#if defined(__MSDOS__) && \
    !defined(__PDOS386__) && \
    !defined(__gnu_linux__)
        if (!lng)
        {
            ulvalue &= 0xffff;
        }
#endif
        if ((specifier == 'X') || (specifier == 'x') || (specifier == 'p'))
        {
            base = 16;
        }
        else if (specifier == 'o')
        {
            base = 8;
        }
        else
        {
            base = 10;
        }
        if (specifier == 'p')
        {
#if defined(__MSDOS__) && \
    !defined(__PDOS386__) && \
    !defined(__gnu_linux__)
            precision = 9;
#else
            precision = 8;
#endif
        }
        x = 0;
        while (ulvalue > 0)
        {
            rem = (int)(ulvalue % base);
            if (rem < 10)
            {
                work[x] = (char)('0' + rem);
            }
            else
            {
                if ((specifier == 'X') || (specifier == 'p'))
                {
                    work[x] = (char)('A' + (rem - 10));
                }
                else
                {
                    work[x] = (char)('a' + (rem - 10));
                }
            }
            x++;
#if defined(__MSDOS__) && \
    !defined(__PDOS386__) && \
    !defined(__gnu_linux__)
            if ((x == 4) && (specifier == 'p'))
            {
                work[x] = ':';
                x++;
            }
#endif
            ulvalue = ulvalue / base;
        }
#if defined(__MSDOS__) && \
    !defined(__PDOS386__) && \
    !defined(__gnu_linux__)
        if (specifier == 'p')
        {
            while (x < 5)
            {
                work[x] = (x == 4) ? ':' : '0';
                x++;
            }
        }
#endif
        while (x < precision)
        {
            work[x] = '0';
            x++;
        }
        if (neg)
        {
            work[x++] = '-';
        }
        else if (flagPlus)
        {
            work[x++] = '+';
        }
        else if (flagSpace)
        {
            work[x++] = ' ';
        }
        if (flagZero)
        {
            fillCh = '0';
        }
        else
        {
            fillCh = ' ';
        }
        y = x;
        if (!flagMinus)
        {
            while (y < width)
            {
                outch(fillCh);
                extraCh++;
                y++;
            }
        }
        if (flagHash && (toupper((unsigned char)specifier) == 'X'))
        {
            outch('0');
            outch('x');
            extraCh += 2;
        }
        x--;
        while (x >= 0)
        {
            outch(work[x]);
            extraCh++;
            x--;
        }
        if (flagMinus)
        {
            while (y < width)
            {
                outch(fillCh);
                extraCh++;
                y++;
            }
        }
    }
    else if (strchr("eEgGfF", specifier) != NULL && specifier != 0)
    {
        if (precision < 0)
        {
            precision = 6;
        }
        vdbl = va_arg(*arg, double);
        dblcvt(vdbl, specifier, width, precision, work);   /* 'e','f' etc. */
        slen = strlen(work);
        if ((flagSpace || flagPlus) && (work[0] != '-'))
        {
            slen++;
            memmove(work + 1, work, slen);
            if (flagSpace)
            {
                work[0] = ' ';
            }
            else if (flagPlus)
            {
                work[0] = '+';
            }
        }
        if (fq == NULL)
        {
            memcpy(s, work, slen);
            s += slen;
        }
        else
        {
            fputs(work, fq);
        }
        extraCh += slen;
    }
    else if (specifier == 's')
    {
        svalue = va_arg(*arg, char *);
        fillCh = ' ';
        if (precision > 0)
        {
            char *p;

            p = memchr(svalue, '\0', precision);
            if (p != NULL)
            {
                length = (int)(p - svalue);
            }
            else
            {
                length = precision;
            }
        }
        else if (precision < 0)
        {
            length = strlen(svalue);
        }
        else
        {
            length = 0;
        }
        if (!flagMinus)
        {
            if (length < width)
            {
                extraCh += (width - length);
                for (x = 0; x < (width - length); x++)
                {
                    outch(fillCh);
                }
            }
        }
        for (x = 0; x < length; x++)
        {
            outch(svalue[x]);
        }
        extraCh += length;
        if (flagMinus)
        {
            if (length < width)
            {
                extraCh += (width - length);
                for (x = 0; x < (width - length); x++)
                {
                    outch(fillCh);
                }
            }
        }
    }
    *formt = format;
    return (extraCh);
}

__PDPCLIB_API__ int fputc(int c, FILE *stream)
{
    char buf[1];

    stream = __INTFILE(stream);

#if !defined(__MVS__) && !defined(__CMS__)
    stream->quickBin = 0;
    if ((stream->upto < (stream->endbuf - 2))
        && (stream->bufTech != _IONBF))
    {
        if (stream->textMode)
        {
            if (c == '\n')
            {
                if (stream->bufTech == _IOFBF)
                {
                    *stream->upto++ = '\r';
                    *stream->upto++ = '\n';
                }
                else
                {
                    buf[0] = (char)c;
                    if (fwrite(buf, 1, 1, stream) != 1)
                    {
                        return (EOF);
                    }
                }
            }
            else
            {
                *stream->upto++ = (char)c;
            }
        }
        else
        {
            *stream->upto++ = (char)c;
        }
    }
    else
#endif
    {
        buf[0] = (char)c;
        if (fwrite(buf, 1, 1, stream) != 1)
        {
            return (EOF);
        }
    }
    return (c);
}

#if !defined(__MVS__) && !defined(__CMS__)
__PDPCLIB_API__ int fputs(const char *s, FILE *stream)
{
    size_t len;
    size_t ret;

    stream = __INTFILE(stream);

    len = strlen(s);
    ret = fwrite(s, len, 1, stream);
    if (ret != 1) return (EOF);
    else return (0);
}
#endif

__PDPCLIB_API__ int remove(const char *filename)
{
    int ret;
#ifdef __OS2__
    APIRET rc;
#endif
#ifdef __WIN32__
    BOOL rc;
#endif

#ifdef __OS2__
    rc = DosDelete((PSZ)filename);
    if (rc != 0)
    {
        ret = 1;
        errno = rc;
    }
    else
    {
        ret = 0;
    }
#endif
#ifdef __WIN32__
    rc = DeleteFile(filename);
    if (!rc)
    {
        ret = 1;
        errno = GetLastError();
    }
    else
    {
        ret = 0;
    }
#endif
#ifdef __MSDOS__
    __remove(filename);
    ret = 0;
#endif
#ifdef __MVS__
    char buf[FILENAME_MAX + 50];
    char *p;

    sprintf(buf, " DELETE %s", filename);
    p = buf;
    while (*p != '\0')
    {
       *p = toupper((unsigned char)*p);
       p++;
    }
    ret = __idcams(strlen(buf), buf);
#endif
#ifdef __CMS__
    ret = cmsremove(filename);
#endif
    return (ret);
}

__PDPCLIB_API__ int rename(const char *old, const char *newnam)
{
    int ret;
#ifdef __OS2__
    APIRET rc;
#endif
#ifdef __WIN32__
    BOOL rc;
#endif

#ifdef __OS2__
    rc = DosMove((PSZ)old, (PSZ)newnam);
    if (rc != 0)
    {
        ret = 1;
        errno = rc;
    }
    else
    {
        ret = 0;
    }
#endif
#ifdef __WIN32__
    rc = MoveFile(old, newnam);
    if (!rc)
    {
        ret = 1;
        errno = GetLastError();
    }
    else
    {
        ret = 0;
    }
#endif
#ifdef __MSDOS__
    __rename(old, newnam);
    ret = 0;
#endif
#ifdef __MVS__
    char buf[FILENAME_MAX + FILENAME_MAX + 50];
    char *p;

    sprintf(buf, " ALTER %s NEWNAME(%s)", old, newnam);
    p = buf;
    while (*p != '\0')
    {
       *p = toupper((unsigned char)*p);
       p++;
    }
    ret = __idcams(strlen(buf), buf);
#endif
#ifdef __CMS__
    ret = cmsrename(old, newnam);
#endif
    return (ret);
}

__PDPCLIB_API__ int sprintf(char *s, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vsprintf(s, format, arg);
    va_end(arg);
    return (ret);
}

__PDPCLIB_API__ int vsprintf(char *s, const char *format, va_list arg)
{
    int ret;

    ret = vvprintf(format, arg, NULL, s);
    if (ret >= 0)
    {
        *(s + ret) = '\0';
    }
    return (ret);
}

/*

In fgets, we have the following possibilites...

1. we found a genuine '\n' that terminated the search.
2. we hit the '\n' at the endbuf.
3. we hit the '\n' sentinel.

*/
#if !defined(__MVS__) && !defined(__CMS__)
__PDPCLIB_API__ char *fgets(char *s, int n, FILE *stream)
{
    char *p;
    register char *t;
    register char *u = s;
    int c;
    int processed;
#ifdef __OS2__
    ULONG actualRead;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD actualRead;
    BOOL rc;
#endif
#ifdef __MSDOS__
    size_t actualRead;
    int errind;
#endif

    stream = __INTFILE(stream);

    if (stream->quickText)
    {
        p = stream->upto + n - 1;
        t = stream->upto;
        if (p < stream->endbuf)
        {
            c = *p;
            *p = '\n';
#if defined(__OS2__) || defined(__WIN32__)
            if (n < 8)
            {
#endif
                while ((*u++ = *t++) != '\n') ; /* tight inner loop */
#if defined(__OS2__) || defined(__WIN32__)
            }
            else
            {
                register unsigned int *i1;
                register unsigned int *i2;
                register unsigned int z;

                i1 = (unsigned int *)t;
                i2 = (unsigned int *)u;
                while (1)
                {
                    z = *i1;
                    if ((z & 0xffU) == '\n') break;
                    z >>= 8;
                    if ((z & 0xffU) == '\n') break;
                    z >>= 8;
                    if ((z & 0xffU) == '\n') break;
                    z >>= 8;
                    if ((z & 0xffU) == '\n') break;
                    *i2++ = *i1++;
                }
                t = (char *)i1;
                u = (char *)i2;
                while ((*u++ = *t++) != '\n') ;
            }
#endif
            *p = (char)c;
            if (t <= p)
            {
                if (*(t - 2) == '\r') /* t is protected, u isn't */
                {
                    *(u - 2) = '\n';
                    *(u - 1) = '\0';
                }
                else
                {
                    *u = '\0';
                }
                stream->upto = t;
                return (s);
            }
            else
            {
                processed = (int)(t - stream->upto) - 1;
                stream->upto = t - 1;
                u--;
            }
        }
        else
        {
            while ((*u++ = *t++) != '\n') ; /* tight inner loop */
            if (t <= stream->endbuf)
            {
                if (*(t - 2) == '\r') /* t is protected, u isn't */
                {
                    *(u - 2) = '\n';
                    *(u - 1) = '\0';
                }
                else
                {
                    *u = '\0';
                }
                stream->upto = t;
                return (s);
            }
            else
            {
                processed = (int)(t - stream->upto) - 1;
                stream->upto = t - 1;
                u--;
            }
        }
    }
    else
    {
        processed = 0;
    }

    if (n < 1)
    {
        return (NULL);
    }
    if (n < 2)
    {
        *u = '\0';
        return (s);
    }
    if (stream->ungetCh != -1)
    {
        processed++;
        *u++ = (char)stream->ungetCh;
        stream->ungetCh = -1;
    }
    while (1)
    {
        t = stream->upto;
        p = stream->upto + (n - processed) - 1;
        if (p < stream->endbuf)
        {
            c = *p;
            *p = '\n';
        }
        if (stream->noNl)
        {
            while (((*u++ = *t) != '\n') && (*t++ != '\r')) ;
            if (*(u - 1) == '\n')
            {
                t++;
            }
            else
            {
                u--;
                while ((*u++ = *t++) != '\n') ;
            }
        }
        else
        {
            while ((*u++ = *t++) != '\n') ; /* tight inner loop */
        }
        if (p < stream->endbuf)
        {
            *p = (char)c;
        }
        if (((t <= p) && (p < stream->endbuf))
           || ((t <= stream->endbuf) && (p >= stream->endbuf)))
        {
            if (stream->textMode)
            {
                if (stream->noNl)
                {
                    if ((*(t - 1) == '\r') || (*(t - 1) == '\n'))
                    {
                        *(u - 1) = '\0';
                    }
                    else
                    {
                        *u = '\0';
                    }
                }
                else if (*(t - 2) == '\r') /* t is protected, u isn't */
                {
                    *(u - 2) = '\n';
                    *(u - 1) = '\0';
                }
                else
                {
                    *u = '\0';
                }
            }
            stream->upto = t;
            if (stream->textMode)
            {
                stream->quickText = 1;
            }
            return (s);
        }
        else if (((t > p) && (p < stream->endbuf))
                 || ((t > stream->endbuf) && (p >= stream->endbuf)))
        {
            int leave = 1;

            if (stream->textMode)
            {
                if (t > stream->endbuf)
                {
                    if ((t - stream->upto) > 1)
                    {
                        if (*(t - 2) == '\r') /* t is protected, u isn't */
                        {
                            processed -= 1; /* preparation for add */
                        }
                    }
                    leave = 0;
                }
                else
                {
                    if ((*(t - 2) == '\r') && (*(t - 1) == '\n'))
                    {
                        *(u - 2) = '\n';
                        *(u - 1) = '\0';
                    }
                    else
                    {
                        t--;
                        *(u - 1) = '\0';
                    }
                }
            }
            else if (t > stream->endbuf)
            {
                leave = 0;
            }
            else
            {
                *u = '\0';
            }
            if (leave)
            {
                stream->upto = t;
                if (stream->textMode)
                {
                    stream->quickText = 1;
                }
                return (s);
            }
        }
        processed += (int)(t - stream->upto) - 1;
        u--;
        stream->bufStartR += (stream->endbuf - stream->fbuf);
#ifdef __OS2__
        rc = DosRead(stream->hfile, stream->fbuf, stream->szfbuf, &actualRead);
        if (rc != 0)
        {
            actualRead = 0;
            stream->errorInd = 1;
            errno = rc;
        }
#endif
#ifdef __WIN32__
        rc = ReadFile(stream->hfile,
                      stream->fbuf,
                      stream->szfbuf,
                      &actualRead,
                      NULL);
        if (!rc)
        {
            actualRead = 0;
            stream->errorInd = 1;
            errno = GetLastError();
        }
#endif
#ifdef __MSDOS__
        actualRead = __read(stream->hfile,
                            stream->fbuf,
                            stream->szfbuf,
                            &errind);
        if (errind)
        {
            errno = actualRead;
            actualRead = 0;
            stream->errorInd = 1;
        }
#endif
        stream->endbuf = stream->fbuf + actualRead;
        *stream->endbuf = '\n';
        if (actualRead == 0)
        {
            *u = '\0';
            if ((u - s) <= 1)
            {
                stream->eofInd = 1;
                return (NULL);
            }
            else
            {
                return (s);
            }
        }
        stream->upto = stream->fbuf;
    }
}
#endif

__PDPCLIB_API__ int ungetc(int c, FILE *stream)
{
    stream = __INTFILE(stream);

    if ((stream->ungetCh != -1) || (c == EOF))
    {
        return (EOF);
    }
    stream->ungetCh = (unsigned char)c;
    stream->quickText = 0;
    stream->quickBin = 0;
    return ((unsigned char)c);
}

__PDPCLIB_API__ int fgetc(FILE *stream)
{
    unsigned char x[1];
    size_t ret;

    stream = __INTFILE(stream);

    ret = fread(x, 1, 1, stream);
    if (ret == 0)
    {
        return (EOF);
    }
    return ((int)x[0]);
}

__PDPCLIB_API__ int fseek(FILE *stream, long int offset, int whence)
{
    long oldpos;
    long newpos;
#ifdef __OS2__
    ULONG retpos;
    APIRET rc;
#endif
#ifdef __WIN32__
    DWORD retpos;
#endif
#ifdef __MSDOS__
    int ret;
#endif

    stream = __INTFILE(stream);

    oldpos = stream->bufStartR + (stream->upto - stream->fbuf);
    if (stream->mode == __WRITE_MODE)
    {
        fflush(stream);
    }
    if (whence == SEEK_SET)
    {
        newpos = offset;
    }
    else if (whence == SEEK_CUR)
    {
        newpos = oldpos + offset;
    }

    if (whence == SEEK_END)
    {
        char buf[1000];

        if (stream->mode == __WRITE_MODE)
        {
            fseek(stream, oldpos, SEEK_SET);
        }
        while (fread(buf, sizeof buf, 1, stream) == 1)
        {
            /* do nothing */
        }
    }
    else if ((newpos >= stream->bufStartR)
        && (newpos < (stream->bufStartR + (stream->endbuf - stream->fbuf)))
        && !stream->update
        && (stream->mode == __READ_MODE))
    {
        stream->upto = stream->fbuf + (size_t)(newpos - stream->bufStartR);
    }
    else
    {
#ifdef __OS2__
        rc = DosSetFilePtr(stream->hfile, newpos, FILE_BEGIN, &retpos);
        if ((rc != 0) || (retpos != newpos))
        {
            errno = rc;
            return (-1);
        }
        stream->endbuf = stream->fbuf + stream->szfbuf;
        if (stream->mode == __READ_MODE)
        {
            stream->upto = stream->endbuf;
            stream->bufStartR = newpos - stream->szfbuf;
        }
        else
        {
            stream->upto = stream->fbuf;
            stream->bufStartR = newpos;
        }
#endif
#ifdef __WIN32__
        retpos = SetFilePointer(stream->hfile, newpos, NULL, FILE_BEGIN);
        if (retpos != newpos)
        {
            errno = GetLastError();
            return (-1);
        }
        stream->endbuf = stream->fbuf + stream->szfbuf;
        if (stream->mode == __READ_MODE)
        {
            stream->upto = stream->endbuf;
            stream->bufStartR = newpos - stream->szfbuf;
        }
        else
        {
            stream->upto = stream->fbuf;
            stream->bufStartR = newpos;
        }
#endif
#ifdef __MSDOS__
        ret = __seek(stream->hfile, newpos, SEEK_SET);
        if (ret) return (ret);
        stream->endbuf = stream->fbuf + stream->szfbuf;
        if (stream->mode == __READ_MODE)
        {
            stream->upto = stream->endbuf;
            stream->bufStartR = newpos - stream->szfbuf;
        }
        else
        {
            stream->upto = stream->fbuf;
            stream->bufStartR = newpos;
        }
#endif
#if defined(__MVS__) || defined(__CMS__)
        char fnm[FILENAME_MAX];
        long int x;
        size_t y;
        char buf[1000];

        oldpos = ftell(stream);
        if (newpos < oldpos)
        {
            strcpy(fnm, "dd:");
            strcat(fnm, stream->ddname);
            if (stream->pdsmem[0] != '\0')
            {
                sprintf(fnm + strlen(fnm), "(%s)", stream->pdsmem);
            }
            inseek = 1;
            if (freopen(fnm, stream->modeStr, stream) == NULL)
            {
                stream->errorInd = 1;
                return (-1);
            }
            inseek = 0;
            oldpos = 0;
        }
        y = (newpos - oldpos) % sizeof buf;
        fread(buf, y, 1, stream);
        for (x = oldpos + y; x < newpos; x += sizeof buf)
        {
            fread(buf, sizeof buf, 1, stream);
        }
        if (stream->errorInd)
        {
            return (-1);
        }
#endif
    }
    stream->justseeked = 1;
    stream->quickBin = 0;
    stream->quickText = 0;
    stream->ungetCh = -1;
    return (0);
}

__PDPCLIB_API__ long int ftell(FILE *stream)
{
    stream = __INTFILE(stream);

    return (stream->bufStartR + (stream->upto - stream->fbuf));
}

__PDPCLIB_API__ int fsetpos(FILE *stream, const fpos_t *pos)
{
    stream = __INTFILE(stream);

    fseek(stream, *pos, SEEK_SET);
    return (0);
}

__PDPCLIB_API__ int fgetpos(FILE *stream, fpos_t *pos)
{
    stream = __INTFILE(stream);

    *pos = ftell(stream);
    return (0);
}

__PDPCLIB_API__ void rewind(FILE *stream)
{
    stream = __INTFILE(stream);

    fseek(stream, 0L, SEEK_SET);
    return;
}

__PDPCLIB_API__ void clearerr(FILE *stream)
{
    stream = __INTFILE(stream);

    stream->errorInd = 0;
    stream->eofInd = 0;
    return;
}

__PDPCLIB_API__ void perror(const char *s)
{
    if ((s != NULL) && (*s != '\0'))
    {
        printf("%s: ", s);
    }
    if (errno == 0)
    {
        printf("No error has occurred\n");
    }
    else
    {
        printf("An error has occurred\n");
    }
    return;
}

/*
NULL + F = allocate, setup
NULL + L = allocate, setup
NULL + N = ignore, return success
buf  + F = setup
buf  + L = setup
buf  + N = ignore, return success
*/

__PDPCLIB_API__ int setvbuf(FILE *stream, char *buf, int mode, size_t size)
{
    char *mybuf;

    stream = __INTFILE(stream);

#if defined(__MVS__) || defined(__CMS__)
    /* don't allow mucking around with buffers on MVS or CMS */
    return (0);
#endif

    if (mode == _IONBF)
    {
        stream->bufTech = mode;
        return (0);
    }
    if (buf == NULL)
    {
        if (size < 2)
        {
            return (-1);
        }
        mybuf = malloc(size + 8);
        if (mybuf == NULL)
        {
            return (-1);
        }
    }
    else
    {
        if (size < 10)
        {
            return (-1);
        }
        mybuf = buf;
        stream->theirBuffer = 1;
        size -= 8;
    }
    if (!stream->permfile)
    {
        free(stream->intBuffer);
    }
    stream->intBuffer = mybuf;
    stream->fbuf = stream->intBuffer + 2;
    *stream->fbuf++ = '\0';
    *stream->fbuf++ = '\0';
    stream->szfbuf = size;
    stream->endbuf = stream->fbuf + stream->szfbuf;
    *stream->endbuf = '\n';
    if (stream->mode == __WRITE_MODE)
    {
        stream->upto = stream->fbuf;
    }
    else
    {
        stream->upto = stream->endbuf;
    }
    stream->bufTech = mode;
    if (!stream->textMode && (stream->bufTech == _IOLBF))
    {
        stream->quickBin = 0;
    }
    return (0);
}

__PDPCLIB_API__ int setbuf(FILE *stream, char *buf)
{
    int ret;

    stream = __INTFILE(stream);

    if (buf == NULL)
    {
        ret = setvbuf(stream, NULL, _IONBF, 0);
    }
    else
    {
        ret = setvbuf(stream, buf, _IOFBF, BUFSIZ);
    }
    return (ret);
}

__PDPCLIB_API__ FILE *freopen(const char *filename,
                              const char *mode,
                              FILE *stream)
{
    stream = __INTFILE(stream);

    inreopen = 1;
    fclose(stream);

    myfile = stream;
    fopen(filename, mode);
    if (err && !stream->permfile)
    {
        __userFiles[stream->intFno] = NULL;
        free(stream);
    }
#if defined(__MVS__) || defined(__CMS__)
    else if (err)
    {
        free(stream);
        /* need to protect against the app closing the file
           which it is allowed to */
        if (stream == __stdin)
        {
            __stdin = NULL;
        }
        else if (stream == __stdout)
        {
            __stdout = NULL;
        }
        else if (stream == __stderr)
        {
            __stderr = NULL;
        }
    }
#endif
    inreopen = 0;
    if (err)
    {
        return (NULL);
    }
    return (stream);
}

__PDPCLIB_API__ int fflush(FILE *stream)
{
#if defined(__MVS__) || defined(__CMS__)
    if ((stream->mode == __WRITE_MODE) && (stream->upto != stream->fbuf))
    {
        if (stream->reallyu)
        {
            size_t last;

            last = stream->upto - stream->fbuf;
            begwrite(stream, last);
            memcpy(dptr, stream->fbuf, last);
            finwrite(stream);
            stream->upto = stream->fbuf;
        }
        else if ((stream == __stdout) || (stream == __stderr))
        {
            fputc('\n', stream);
        }
    }
#else
#ifdef __OS2__
    APIRET rc;
    ULONG actualWritten;
#endif
#ifdef __WIN32__
    BOOL rc;
    DWORD actualWritten;
#endif
#ifdef __MSDOS__
    int errind;
    size_t actualWritten;
#endif

    stream = __INTFILE(stream);

    if ((stream->upto != stream->fbuf) && (stream->mode == __WRITE_MODE))
    {
#ifdef __OS2__
        rc = DosWrite(stream->hfile,
                     (VOID *)stream->fbuf,
                     (size_t)(stream->upto - stream->fbuf),
                     &actualWritten);
        if (rc != 0)
        {
            stream->errorInd = 1;
            errno = rc;
            return (EOF);
        }
#endif
#ifdef __WIN32__
        rc = WriteFile(stream->hfile,
                       stream->fbuf,
                       (size_t)(stream->upto - stream->fbuf),
                       &actualWritten,
                       NULL);
        if (!rc)
        {
            stream->errorInd = 1;
            errno = GetLastError();
            return (EOF);
        }
#endif
#ifdef __MSDOS__
        actualWritten = __write(stream->hfile,
                                stream->fbuf,
                                (size_t)(stream->upto - stream->fbuf),
                                &errind);
        if (errind)
        {
            stream->errorInd = 1;
            errno = actualWritten;
            return (EOF);
        }
#endif
        stream->bufStartR += actualWritten;
        stream->upto = stream->fbuf;
    }
#endif
    return (0);
}

__PDPCLIB_API__ char *tmpnam(char *s)
{
#if defined(__MVS__) || defined(__CMS__)
    static char buf[] = "dd:ZZZZZZZA";
#else
    static char buf[] = "ZZZZZZZA.$$$";
#endif

#if defined(__MVS__) || defined(__CMS__)
    buf[10]++;
#else
    buf[7]++;
#endif
    if (s == NULL)
    {
        return (buf);
    }
    strcpy(s, buf);
    return (s);
}

__PDPCLIB_API__ FILE *tmpfile(void)
{
#if defined(__MVS__) || defined(__CMS__)
    return (fopen("dd:ZZZZZZZA", "wb+"));
#else
    return (fopen("ZZZZZZZA.$$$", "wb+"));
#endif
}

__PDPCLIB_API__ int fscanf(FILE *stream, const char *format, ...)
{
    va_list arg;
    int ret;

    stream = __INTFILE(stream);

    va_start(arg, format);
    ret = vvscanf(format, arg, stream, NULL);
    va_end(arg);
    return (ret);
}

__PDPCLIB_API__ int scanf(const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vvscanf(format, arg, __stdin, NULL);
    va_end(arg);
    return (ret);
}

__PDPCLIB_API__ int sscanf(const char *s, const char *format, ...)
{
    va_list arg;
    int ret;

    va_start(arg, format);
    ret = vvscanf(format, arg, NULL, s);
    va_end(arg);
    return (ret);
}

/* vvscanf - the guts of the input scanning */
/* several mods by Dave Edwards */
static int vvscanf(const char *format, va_list arg, FILE *fp, const char *s)
{
    int ch;
    int fin = 0;
    int cnt = 0;
    char *cptr;
    int *iptr;
    unsigned int *uptr;
    long *lptr;
    unsigned long *luptr;
    short *hptr;
    unsigned short *huptr;
    double *dptr;
    float *fptr;
    long startpos;
    const char *startp;
    int skipvar; /* nonzero if we are skipping this variable */
    int modlong;   /* nonzero if "l" modifier found */
    int modshort;   /* nonzero if "h" modifier found */
    int width; /* maximum chars to read */
    int informatitem;  /* nonzero if % format item started */
           /* informatitem is 1 if we have processed "%l" but not the
              type letter (s,d,e,f,g,...) yet. */

    if (fp != NULL)
    {
        startpos = ftell(fp);
    }
    else
    {
        startp = s;
    }
    inch();
    informatitem = 0;   /* init */
    /* don't bail out at end of data as there may be a %n */
    /*if ((fp != NULL && ch == EOF) || (fp == NULL && ch == 0)) return EOF; */
                               /* initially at EOF or end of string */
    while (!fin)
    {
        if (*format == '\0')
        {
            fin = 1;
        }
        else if (*format == '%' || informatitem)
        {
            if(*format=='%')   /* starting a format item */
            {
                format++;
                modlong=0;   /* init */
                modshort=0;
                skipvar = 0;
                width = 0;
                if (*format == '*')
                {
                    skipvar = 1;
                    format++;
                }
            }
            if (*format == '%')   /* %% */
            {
                if (ch != '%') return (cnt);
                inch();
                informatitem=0;
            }
            else if (*format == 'l')
            {
                /* Type modifier: l  (e.g. %ld) */
                modlong=1;
                informatitem=1;
            }
            else if (*format == 'h')
            {
                /* Type modifier: h (short int) */
                modshort=1;
                informatitem=1;
            }
            else if (isdigit((unsigned char)*format))
            {
                width = width * 10 + (*format - '0');
                informatitem = 1;
            }
            else    /* process a type character: */
            {
                informatitem=0;   /* end of format item */
                if (*format == 's')
                {
                    if (!skipvar)
                    {
                        cptr = va_arg(arg, char *);
                    }
                    /* Skip leading whitespace: */
                    while (ch>=0 && isspace(ch)) inch();
                    if ((fp != NULL && ch == EOF) || (fp == NULL && ch == 0))
                                         /* at EOF or end of string */
                    {
                        if (!skipvar)
                        {
                            *cptr = 0;   /* give a null string */
                        }
                        continue;
                    }
                    else
                    {
                        for(;;)
                        {
                            if (isspace(ch)) break;
                            if ((fp != NULL && ch == EOF)
                                || (fp == NULL && ch == 0))
                            {
                                break;
                            }
                            if (!skipvar)
                            {
                                *cptr++ = (char)ch;
                            }
                            inch();
                        }
                        if (!skipvar)
                        {
                            *cptr = '\0';
                        }
                        cnt++;
                    }
                }
                else if (*format == '[')
                {
                    int reverse = 0;
                    int found;
                    const char *first;
                    const char *last;
                    size_t size;
                    size_t mcnt = 0;

                    if (!skipvar)
                    {
                        cptr = va_arg(arg, char *);
                    }
                    format++;
                    if (*format == '^')
                    {
                        reverse = 1;
                        format++;
                    }
                    if (*format == '\0') break;
                    first = format;
                    format++;
                    last = strchr(format, ']');
                    if (last == NULL) return (cnt);
                    size = (size_t)(last - first);
                    while (1)
                    {
                        /* note that C90 doesn't require special
                           processing for '-' so it hasn't been
                           added */
                        found = (memchr(first, ch, size) != NULL);
                        if (found && reverse) break;
                        if (!found && !reverse) break;
                        if (!skipvar)
                        {
                            *cptr++ = (char)ch;
                        }
                        mcnt++;
                        inch();
                        /* if at EOF or end of string, bug out */
                        if ((fp != NULL && ch == EOF)
                            || (fp == NULL && ch == 0))
                        {
                            break;
                        }
                    }
                    if (mcnt > 0)
                    {
                        if (!skipvar)
                        {
                            *cptr++ = '\0';
                        }
                        cnt++;
                    }
                    else
                    {
                        break;
                    }
                    format = last + 1;
                }
                else if (*format == 'c')
                {
                    if (!skipvar)
                    {
                        cptr = va_arg(arg, char *);
                    }
                    if ((fp != NULL && ch == EOF)
                        || (fp == NULL && ch == 0))
                    {
                        /* do nothing */
                    }
                    else
                    {
                        if (!skipvar)
                        {
                            *cptr = ch;
                        }
                        cnt++;
                        inch();
                    }
                }
                else if (*format == 'n')
                {
                    uptr = va_arg(arg, unsigned int *);
                    if (fp != NULL)
                    {
                        *uptr = (unsigned int)(ftell(fp) - startpos);
                    }
                    else
                    {
                        /* we need a -1 because s will point to
                           the character after the NUL */
                        *uptr = (unsigned int)(s - startp - 1);
                    }
                }
                else if (*format == 'd' || *format == 'u'
                         || *format == 'x' || *format == 'o'
                         || *format == 'p'
                         || *format == 'i')
                {
                    int neg = 0;
                    unsigned long x = 0;
                    int undecided = 0;
                    int base = 10;
                    int mcnt = 0;

                    if (*format == 'x') base = 16;
                    else if (*format == 'p') base = 16;
                    else if (*format == 'o') base = 8;
                    else if (*format == 'i') base = 0;
                    if (!skipvar)
                    {
                        if ((*format == 'd') || (*format == 'i'))
                        {
                            if (modlong) lptr = va_arg(arg, long *);
                            else if (modshort) hptr = va_arg(arg, short *);
                            else iptr = va_arg(arg, int *);
                        }
                        else
                        {
                            if (modlong) luptr = va_arg(arg, unsigned long *);
                            else if (modshort) huptr =
                                     va_arg(arg, unsigned short *);
                            else uptr = va_arg(arg, unsigned int *);
                        }
                    }
                    /* Skip leading whitespace: */
                    while (ch>=0 && isspace(ch)) inch();
                    if (ch == '-')
                    {
                        neg = 1;
                        inch();
                    }
                    else if(ch == '+') inch();

                    /* this logic is the same as strtoul so if you
                       change this, change that one too */

                    if (base == 0)
                    {
                        undecided = 1;
                    }
                    while (!((fp != NULL && ch == EOF)
                             || (fp == NULL && ch == 0)))
                    {
                        if (isdigit((unsigned char)ch))
                        {
                            if (base == 0)
                            {
                                if (ch == '0')
                                {
                                    base = 8;
                                }
                                else
                                {
                                    base = 10;
                                    undecided = 0;
                                }
                            }
                            x = x * base + (ch - '0');
                            inch();
                        }
/* DOS has a ':' in the pointer - skip that */
#if defined(__MSDOS__) && \
    !defined(__PDOS386__) && \
    !defined(__gnu_linux__)
                        else if ((*format == 'p') && (ch == ':'))
                        {
                            inch();
                        }
#endif
                        else if (isalpha((unsigned char)ch))
                        {
                            if ((ch == 'X') || (ch == 'x'))
                            {
                                if ((base == 0) || ((base == 8) && undecided))
                                {
                                    base = 16;
                                    undecided = 0;
                                    inch();
                                }
                                else if (base == 16)
                                {
                                    /* hex values are allowed to have an
                                       optional 0x */
                                    inch();
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else if (base <= 10)
                            {
                                break;
                            }
                            else
                            {
                                x = x * base +
                                    (toupper((unsigned char)ch) - 'A') + 10;
                                inch();
                            }
                        }
                        else
                        {
                            break;
                        }
                        mcnt++;
                    }

                    /* end of strtoul logic */

                    /* If we didn't get any characters, don't go any
                       further */
                    if (mcnt == 0)
                    {
                        break;
                    }


                    if (!skipvar)
                    {
                        if ((*format == 'd') || (*format == 'i'))
                        {
                            long lval;

                            if (neg)
                            {
                                lval = (long)-x;
                            }
                            else
                            {
                                lval = (long)x;
                            }
                            if (modlong) *lptr=lval;
                                /* l modifier: assign to long */
                            else if (modshort) *hptr = (short)lval;
                                /* h modifier */
                            else *iptr=(int)lval;
                        }
                        else
                        {
                            if (modlong) *luptr = (unsigned long)x;
                            else if (modshort) *huptr = (unsigned short)x;
                            else *uptr = (unsigned int)x;
                        }
                    }
                    cnt++;
                }
                else if (*format=='e' || *format=='f' || *format=='g' ||
                         *format=='E' || *format=='G')
                {
                    /* Floating-point (double or float) input item */
                    int negsw1,negsw2,dotsw,expsw,ndigs1,ndigs2,nfdigs;
                    int ntrailzer,expnum,expsignsw;
                    double fpval,pow10;

                    if (!skipvar)
                    {
                        if (modlong) dptr = va_arg(arg, double *);
                        else fptr = va_arg(arg, float *);
                    }
                    negsw1=0;   /* init */
                    negsw2=0;
                    dotsw=0;
                    expsw=0;
                    ndigs1=0;
                    ndigs2=0;
                    nfdigs=0;
                    ntrailzer=0;  /* # of trailing 0's unaccounted for */
                    expnum=0;
                    expsignsw=0;  /* nonzero means done +/- on exponent */
                    fpval=0.0;
                    /* Skip leading whitespace: */
                    while (ch>=0 && isspace(ch)) inch();
                    if (ch=='-')
                    {
                        negsw1=1;
                        inch();
                    }
                    else if (ch=='+') inch();
                    while (ch>0)
                    {
                        if (ch=='.' && dotsw==0 && expsw==0) dotsw=1;
                        else if (isdigit(ch))
                        {
                            if(expsw)
                            {
                                ndigs2++;
                                expnum=expnum*10+(ch-'0');
                            }
                            else
                            {
                                /* To avoid overflow or loss of precision,
                                   skip leading and trailing zeros unless
                                   really needed. (Automatic for leading
                                   0's, since 0.0*10.0 is 0.0) */
                                ndigs1++;
                                if (dotsw) nfdigs++;
                                if (ch=='0' && fpval!=0.)
                                {
                                    /* Possible trailing 0 */
                                    ntrailzer++;
                                }
                                else
                                {
                                    /* Account for any preceding zeros */
                                    while (ntrailzer>0)
                                    {
                                        fpval*=10.;
                                        ntrailzer--;
                                    }
                                    fpval=fpval*10.0+(ch-'0');
                                }
                            }
                        }
                        else if ((ch=='e' || ch=='E') && expsw==0) expsw=1;
                        else if ((ch=='+' || ch=='-') && expsw==1
                                 && ndigs2==0 && expsignsw==0)
                        {
                            expsignsw=1;
                            if (ch=='-') negsw2=1;
                        }
                        else break;   /* bad char: end of input item */
                        inch();
                    }
                    /* don't finish at end of input there may be a %n */
                    /*if ((fp != NULL && ch == EOF)
                        || (fp == NULL && ch == 0)) fin=1;*/
                    /* Check for a valid fl-pt value: */
                    if (ndigs1==0 || (expsw && ndigs2==0)) return(cnt);
                    /* Complete the fl-pt value: */
                    if (negsw2) expnum=-expnum;
                    expnum+=ntrailzer-nfdigs;
                    if (expnum!=0 && fpval!=0.0)
                    {
                        negsw2=0;
                        if (expnum<0)
                        {
                            expnum=-expnum;
                            negsw2=1;
                        }
                        /* Multiply or divide by 10.0**expnum, using
                           bits of expnum (fast method) */
                        pow10=10.0;
                        for (;;)
                        {
                            if (expnum & 1)     /* low-order bit */
                            {
                                if (negsw2) fpval/=pow10;
                                else fpval*=pow10;
                            }
                            expnum>>=1;   /* shift right 1 bit */
                            if (expnum==0) break;
                            pow10*=pow10;   /* 10.**n where n is power of 2 */
                        }
                    }
                    if (negsw1) fpval=-fpval;
                    if (!skipvar)
                    {
                        /* l modifier: assign to double */
                        if (modlong) *dptr=fpval;
                        else *fptr=(float)fpval;
                    }
                    cnt++;
                }
            }
        }
        else if (isspace((unsigned char)(*format)))
        {
            /* Whitespace char in format string: skip next whitespace
               chars in input data. This supports input of multiple
               data items. */
            while (ch>=0 && isspace(ch))
            {
                inch();
            }
        }
        else   /* some other character in the format string */
        {
            if (ch != *format) return (cnt);
            inch();
        }
        format++;
        /* don't bail out at end of string as there may be a %n */
        /*if ((fp != NULL && ch == EOF) || (fp == NULL && ch == 0)) fin = 1;*/
            /* EOF */
    }
    if (fp != NULL) ungetc(ch, fp);
    return (cnt);
}

__PDPCLIB_API__ char *gets(char *s)
{
    char *ret;

    __stdin->quickText = 0;
    __stdin->noNl = 1;
    ret = fgets(s, INT_MAX, __stdin);
    __stdin->noNl = 0;
    __stdin->quickText = 1;
    return (ret);
}

__PDPCLIB_API__ int puts(const char *s)
{
    int ret;

    ret = fputs(s, __stdout);
    if (ret == EOF)
    {
        return (ret);
    }
    return (putc('\n', __stdout));
}

/* The following functions are implemented as macros */

#undef getchar
#undef putchar
#undef getc
#undef putc
#undef feof
#undef ferror

__PDPCLIB_API__ int getc(FILE *stream)
{
    stream = __INTFILE(stream);

    return (fgetc(stream));
}

__PDPCLIB_API__ int putc(int c, FILE *stream)
{
    stream = __INTFILE(stream);

    return (fputc(c, stream));
}

__PDPCLIB_API__ int getchar(void)
{
    return (getc(__stdin));
}

__PDPCLIB_API__ int putchar(int c)
{
    return (putc(c, __stdout));
}

__PDPCLIB_API__ int feof(FILE *stream)
{
    stream = __INTFILE(stream);

    return (stream->eofInd);
}

__PDPCLIB_API__ int ferror(FILE *stream)
{
    stream = __INTFILE(stream);

    return (stream->errorInd);
}

#if 0
Design of MVS i/o routines

The broad objectives of the MVS implementation are as follows:

1. An application doing a binary fread equal to LRECL should
get speed equivalent to doing the GET macro.

2. An application doing a text fgets equal to or greater than
LRECL should get speed equivalent to the GET macro.

3. Variable-block files are encapsulated in terms of RDW files.
RDW files are what is produced by certain implementations of
ftp when the "rdw" option is chosen. Data is stored on the PC
or whatever as a 2-byte big-endian length, then 2 NUL bytes,
then the data. See the S/380 documentation that comes with
MVS/380 for more information on this format. So a binary read
of a V or VB file will produce a RDW stream (basically, BDWs
are stripped, but not RDWs).

4. If a binary copy is done from a V dataset to a U dataset,
the RDW data stream will be preserved exactly. If the U data
set is subsequently copied to an F dataset, there will
necessarily be NUL-padding. If this dataset is then copied
to a V dataset, the extraneous NULs (which comprise an
invalid RDW) will be silently ignored/stripped.

5. If a text copy is done from a V dataset to a U dataset,
the U dataset will get x'15' (EBCDIC newline) characters
added. The RDW will be stripped. Trailing spaces will be
preserved. With one exception - a single blank character
on a line will be removed.  If this dataset is then copied
to a F dataset, there will be trailing spaces added to fit
the LRECL. If this dataset is then copied to a V dataset,
the trailing spaces will all be truncated. If a line is
empty, a single blank character will be inserted.

6. If a long line is being written in text mode, it will
be silently truncated regardless of whether the output file
is RECFM=V or F. In binary mode, when writing to a RECFM=F,
the data simply gets wrapped to the next record. For a binary
write to a RECFM=V where the RDW signals a length greater
than the LRECL, the record will be silently truncated. If
writing to a RECFM=V in binary mode with insufficient data
to match the RDW, it is considered an error. Similarly,
more data than the RDW will cause a new record to be
started. An invalid RDW in the data stream is considered
a write error.

7. In RECFM=U datasets, the block boundary is always ignored.
When the application writes a newline character to the data
stream, it is treated as just another character and dutifully
written out. Newlines are never added or stripped by the
C library when a block boundary is encountered - not even in
text mode. This marks a break from IBMs behaviour and is
required in order to be able to read a RECFM=U in binary
mode (e.g. the way zip would) and still preserve newline
characters if the file being read happens to be a text file
(as opposed to e.g. another zip file - something zip has
no way of knowing). NULs encountered when reading
a RECFM=U in text mode may be stripped. Similarly, trailing
NULs in the application data stream are stripped. This way,
someone doing a binary copy of a file, and who has stored
it in a RECFM=F dataset (where NUL padding is necessary),
and then has copied it into a RECFM=U (where NULs must
necessarily be preserved if doing binary copies) will be
stripped by the first person who does a text read or
write.

8. DCB information provided by the user in JCL, or on a
dataset, always has priority and the C library will adjust
around that. Only if there is no existing DCB information
available anywhere will the C library provide a default,
which is RECFM=VB,LRECL=255,BLKSIZE=6233 for text, and
RECFM=U,LRECL=0,BLKSIZE=6233 for binary. This blocksize
is traditionally considered to be the universal blocksize.

9. An open of a PDS with no member given will read the
directory. Any attempt to open a PDS directory for writing
will fail.

10. RECFM=U allows you to preserve the exact length of
data. RECFM=FB with a LRECL of 1 also achieves this, but
is much more overhead. A special exception may be made in
the future for binary reading of FB datasets to provide
the same performance as RECFM=U.

11. Data is processed by the C library one record at a time.
There is an intention to change to block reading in the
future now that the assembler (for MVS at least) has that
flexibility.


The implementation has been based around 4 different processing
concepts:

1. Fixed text.
2. Fixed binary.
3. Variable text.
4. Variable binary.

RECFM=U was grafted on in terms of a faked variable binary
for reading and a faked fixed binary for writing. There is
a "reallyu" to record the fact that it was really U, and
at various points the processing changes slightly to cope
with that. There is also a "reallyt" variable that notes
the fact that it was originally a text mode request, but
that has been switched (to avoid translation of newlines
into spaces or RDW etc).

The code has been designed to work in both locate mode
and in move mode, although it would be rare for anyone to
want to use locate mode, and support for that may drop at
some point.


The main assembler functions are as follows (note that
__aopen in C becomes @@AOPEN in assembler):

void *__aopen(const char *ddname,
              int *mode,
              int *recfm,
              int *lrecl,
              int *blksize,
              void **asmbuf,
              const char *member);

This function opens an 8-character (right-padded with spaces)
DDNAME. For dynamically-allocated datasets, a previous call
to __dynal would have been done to a generated ddname of the
form PDP001HD where 001 corresponds to a spare slot. The
mode is passed by address. It is typically simply set to
read or write, but if it is set to read, blocked, then the
assembler routines have the option of setting this to just
read, e.g. if the device is a terminal and block mode is
inappropriate.

Mode values are as follows:
0 = input (read)
1 = output (write)
2 = update (read and write, initally read)
3 = append
4 = inout = read and write, initially read (same as update)
5 = outin = write, then reread

Additional mode flags:
0x08 = Use EXCP if input file is tape.
0x10 = Use block mode.
0x80 = Use GETLINE/PUTLINE if TSO terminal detected

recfm values are:
0 = fixed
1 = variable
2 = undefined
And again, the C program provides defaults but the assembler
function has the final say.

lrecl = default record/line length set by caller, with
assembler deciding what to really do.

blksize - default block size set by caller, assembler deciding
what to really use.

asmbuf - if file is opened in write mode, in the normal move
mode, then this will be set to point to a buffer large enough
to store the lrecl. Storage will be below the line so it is
suitable for doing I/O from. Buffer will be freed when the
dataset is closed.

member - pointer to a PDS member to be opened. Member should
be 8 characters, padded with spaces. If member is set to NULL,
then this open is not for a member of a PDS (so this parameter
is probably normally NULL).

__aopen returns a "handle" on success, or a negative value
(when cast to int) on failure. These values will probably
be unclumped in the future.


int __aread(void *handle, void *buf, size_t *len);

This function takes the handle previously returned from __aopen
and reads into the provided buffer a single record. It is
assumed that the buffer is big enough to hold the LRECL
previously returned by __aopen. *len will contain the length
of the actual record returned, e.g. if RECFM=U, then while
reading each record (block), the length might change.
In the case of RECFM=V, the record includes a RDW.

__aread returns 0 on success, non-zero on failure.


int __awrite(void *handle, unsigned char **buf, size_t *sz);

This function takes the handle previously returned from __aopen
and writes the buffer pointed to by *buf. If operating in locate
mode, it actually sets the *buf to where to write to, so the
application can subsequently write there. *sz provides the
length of the data to write, which is particularly necessary
for RECFM=U where there is no other way to know the length.
In the future, the assembler may update the size to reflect
actual written in the case of a partial write.

__awrite returns 0 on success, non-zero for failure.


void __aclose(void *handle);

This function takes the handle previously returned from __aopen
and closes the file and releases any buffers that were allocated
in the open.



Here is some old documentation that might be worth updating
one day:

in/out function rec-type mode   method
in     fread    fixed    bin    loop reading, remember remainder
in     fread    fixed    text   loop reading + truncing, remember rem
in     fread    var      bin    loop reading (+ len), remember remainder
in     fread    var      text   loop reading (+ len), remember remainder
in     fgets    fixed    bin    read, scan, remember remainder
in     fgets    fixed    text   read, trunc, remember remainder
in     fgets    var      bin    read, scan, rr
in     fgets    var      text   read, rr
in     fgetc    fixed    bin    read, rr
in     fgetc    fixed    text   read, trunc, rr
in     fgetc    var      bin    read, rr
in     fgetc    var      text   read, rr

out    fwrite   fixed    bin    loop doing put, rr
out    fwrite   fixed    text   search newline, copy + pad, put, rr
out    fwrite   var      bin    if nelem != 1 copy to max lrecl
out    fwrite   var      text   loop search nl, put, rr
out    fputs    fixed    bin    loop doing put, rr
out    fputs    fixed    text   search newline, copy + pad, put, rr
out    fputs    var      bin    put
out    fputs    var      text   search newline, put, copy rem
out    fputc    fixed    bin    copy to rr until rr == lrecl
out    fputc    fixed    text   copy to rr until newline, then pad
out    fputc    var      bin    copy to rr until rr == lrecl
out    fputc    var      text   copy to rr until newline


fread: if quickbin, if read elem size == lrecl, doit
fgets: if variable record + no remainder
       if buffer > record size, copy + add newline
#endif

#if defined(__MVS__) || defined(__CMS__)
__PDPCLIB_API__ char *fgets(char *s, int n, FILE *stream)
{
    unsigned char *eptr;
    size_t len;
    int cnt;
    int c;

    if (stream->quickText)
    {
        if (__aread(stream->hfile, &dptr) != 0)
        {
            stream->eofInd = 1;
            stream->quickText = 0;
            return (NULL);
        }
        len = ((dptr[0] << 8) | dptr[1]) - 4;
        if ((len == 1) && (dptr[4] == ' '))
        {
            len = 0;
        }
        if (n > (len + 1))
        {
            memcpy(s, dptr + 4, len);
            memcpy(s + len, "\n", 2);
            stream->bufStartR += len + 1;
            return (s);
        }
        else
        {
            memcpy(stream->fbuf, dptr + 4, len);
            stream->upto = stream->fbuf;
            stream->endbuf = stream->fbuf + len;
            *(stream->endbuf++) = '\n';
            stream->quickText = 0;
        }
    }

    if (stream->eofInd)
    {
        return (NULL);
    }

    switch (stream->style)
    {
        case FIXED_TEXT:
            if ((stream->endbuf == stream->fbuf)
                && (n > (stream->lrecl + 2)))
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    return (NULL);
                }
                eptr = dptr + stream->lrecl - 1;
                while ((*eptr == ' ') && (eptr >= dptr))
                {
                    eptr--;
                }
                eptr++;
                memcpy(s, dptr, eptr - dptr);
                if (stream->noNl)
                {
                    s[eptr - dptr] = '\0';
                }
                else
                {
                    memcpy(s + (eptr - dptr), "\n", 2);
                }
                stream->bufStartR += (eptr - dptr) + 1;
                return (s);
            }
            break;

        default:
            break;

    }

    /* Ok, the obvious optimizations have been done,
       so now we switch to the slow generic version */

    n--;
    cnt = 0;
    while (cnt < n)
    {
        c = getc(stream);
        if (c == EOF) break;
        s[cnt] = c;
        if (c == '\n') break;
        cnt++;
    }
    if ((cnt == 0) && (c == EOF)) return (NULL);
    if ((cnt < n) && !stream->noNl)
    {
        s[cnt++] = '\n';
    }
    s[cnt] = '\0';
    return (s);
}

__PDPCLIB_API__ int fputs(const char *s, FILE *stream)
{
    const char *p;
    size_t len;

    if (stream->quickText)
    {
        p = strchr(s, '\n');
        if (p != NULL)
        {
            len = p - s;
            if (len > stream->lrecl)
            {
                len = stream->lrecl;
            }
            begwrite(stream, len + 4);
            memcpy(dptr + 4, s, len);
            dptr[0] = (len + 4) >> 8;
            dptr[1] = (len + 4) & 0xff;
            dptr[2] = 0;
            dptr[3] = 0;
            finwrite(stream);
            stream->bufStartR += (len + 1);
            if (*(p + 1) == '\0')
            {
                return (len + 1);
            }
            s = p + 1;
            stream->quickText = 0;
        }
    }
    switch (stream->style)
    {
        case FIXED_TEXT:
            len = strlen(s);
            if (len > 0)
            {
                len--;
                if (((strchr(s, '\n') - s) == len)
                    && (stream->upto == stream->fbuf)
                    && (len <= stream->lrecl))
                {
                    begwrite(stream, stream->lrecl);
                    memcpy(dptr, s, len);
                    memset(dptr + len, ' ', stream->szfbuf - len);
                    finwrite(stream);
                    stream->bufStartR += len;
                }
                else
                {
                    fwrite(s, len + 1, 1, stream);
                }
            }
            break;

        default:
            len = strlen(s);
            fwrite(s, len, 1, stream);
            break;
    }
    return (0);
}

__PDPCLIB_API__ size_t fwrite(const void *ptr,
                              size_t size,
                              size_t nmemb,
                              FILE *stream)
{
    size_t bytes;
    size_t sz;
    char *p;
    int x;

    if (stream->quickBin)
    {
        if ((nmemb == 1) && (size == stream->lrecl))
        {
            begwrite(stream, stream->lrecl);
            memcpy(dptr, ptr, size);
            finwrite(stream);
            stream->bufStartR += size;
            return (1);
        }
        else
        {
            stream->quickBin = 0;
        }
    }
    switch (stream->style)
    {
        case FIXED_BINARY:
            bytes = nmemb * size;
            /* if we've exceed our buffer we need to write out
               a record - but if we haven't written any data to
               our internal buffer yet, don't bother going through
               this code, it'll be handled later. */
            if (((stream->endbuf - stream->upto) <= bytes)
                && (stream->upto != stream->fbuf))
            {
                /* ready to write a record - request some space
                   from MVS */
                begwrite(stream, stream->lrecl);
                sz = stream->endbuf - stream->upto;
                memcpy(dptr, stream->fbuf, stream->szfbuf - sz);
                memcpy(dptr + stream->szfbuf - sz, ptr, sz);
                finwrite(stream);
                ptr = (char *)ptr + sz;
                bytes -= sz;
                stream->upto = stream->fbuf;
                stream->bufStartR += stream->szfbuf;
            }
            /* At this point, the internal buffer is empty if the
               number of bytes to write is still greater than the
               internal buffer. In which case, start writing directly
               to an MVS-provided area. */
            while (bytes >= stream->szfbuf)
            {
                begwrite(stream, stream->lrecl);
                memcpy(dptr, ptr, stream->szfbuf);
                finwrite(stream);
                ptr = (char *)ptr + stream->szfbuf;
                bytes -= stream->szfbuf;
                stream->bufStartR += stream->szfbuf;
            }

            /* RECFM=U to a text file should write up to the most
               recent newline */
            if (stream->line_buf && stream->reallyu && stream->reallyt)
            {
                p = (char *)ptr + bytes - 1;
                /* look for a newline somewhere in this new data */
                /* since we write on both buffer full and newline
                   found conditions */
                while (p >= (char *)ptr)
                {
                    if (*p == '\n') break;
                    p--;
                }
                /* found a newline, write up to this point, including
                   any data that may be in our internal buffer */
                if (p >= (char *)ptr)
                {
                    p++; /* get past the newline */
                    sz = stream->upto - stream->fbuf;
                    stream->upto = stream->fbuf;
                    begwrite(stream, sz + (p - (char *)ptr));
                    memcpy(dptr, stream->fbuf, sz);
                    memcpy(dptr + sz, ptr, (p - (char *)ptr));
                    finwrite(stream);
                    bytes -= (p - (char *)ptr);
                    stream->bufStartR += (p - (char *)ptr);
                    ptr = p;
                    stream->upto = stream->fbuf;
                }
            }

            /* any remainder needs to go into the internal buffer */
            memcpy(stream->upto, ptr, bytes);
            stream->upto += bytes;
            break;

        case VARIABLE_BINARY:
            bytes = nmemb * size;
            while (bytes > 0)
            {
                int fulllen;

                if (stream->errorInd)
                {
                    nmemb = 0;
                    break;
                }
                sz = stream->upto - stream->fbuf;
                if (sz < 4)
                {
                    if ((bytes + sz) < 4)
                    {
                        memcpy(stream->upto, ptr, bytes);
                        stream->upto += bytes;
                        bytes = 0;
                        break;
                    }
                    else
                    {
                        memcpy(stream->upto, ptr, 4 - sz);
                        ptr = (char *)ptr + (4 - sz);
                        bytes -= (4 - sz);
                        stream->upto += (4 - sz);
                        sz = 4;
                        if (memcmp(stream->fbuf + 2, "\0\0", 2) != 0)
                        {
                            stream->errorInd = 1;
                            nmemb = 0;
                            break;
                        }
                        fulllen = (stream->fbuf[0] << 8) | stream->fbuf[1];
                        if (fulllen == 0)
                        {
                            /* here we allow for the possibility that
                               they are copying a data source that has
                               terminating NULs added - so long as all
                               remaining charactes are NUL, it will be
                               allowed. Otherwise we rely on the above
                               validation to catch a problem - checking
                               2 bytes at a time, which shouldn't be a
                               problem since this is only at the end of
                               the file */
                            stream->upto = stream->fbuf + 2;
                            continue;
                        }
                        else if (fulllen < 4)
                        {
                            stream->errorInd = 1;
                            nmemb = 0;
                            break;
                        }
                    }
                }

                /* we have 4 bytes, validated */
                fulllen = (stream->fbuf[0] << 8) | stream->fbuf[1];

                /* If we have enough data, write it out */
                if ((sz + bytes) >= fulllen)
                {
                    /* silently truncate long records to give
                       user more flexibility */
                    if (fulllen > (stream->lrecl + 4))
                    {
                        stream->fbuf[0] = (stream->lrecl + 4) >> 8;
                        stream->fbuf[1] = (stream->lrecl + 4) & 0xff;
                        begwrite(stream, stream->lrecl + 4);
                        if (sz >= (stream->lrecl + 4))
                        {
                            memcpy(dptr, stream->fbuf, stream->lrecl + 4);
                        }
                        else
                        {
                            memcpy(dptr, stream->fbuf, sz);
                            memcpy(dptr + sz, ptr, stream->lrecl + 4 - sz);
                        }
                    }
                    else if (fulllen != 0)
                    {
                        begwrite(stream, fulllen);
                        memcpy(dptr, stream->fbuf, sz);
                        memcpy(dptr + sz, ptr, fulllen - sz);
                    }
                    if (fulllen != 0)
                    {
                        finwrite(stream);
                    }
                    stream->bufStartR += fulllen;
                    stream->upto = stream->fbuf;
                    bytes -= (fulllen - sz);
                    ptr = (char *)ptr + (fulllen - sz);
                }

                /* less data than required, store it, without
                   overflowing our buffer */
                else if ((sz + bytes) > stream->lrecl)
                {
                    memcpy(stream->upto,
                           ptr,
                           stream->lrecl - sz);
                    /* here we allow upto to exceed our buffer.
                       shouldn't be a problem as we never write
                       to that memory. alternative is to make
                       BUFSIZ 64k. */
                    stream->upto += bytes;
                    ptr = (char *)ptr + bytes;
                    bytes = 0;
                }

                /* enough room to fit data */
                else
                {
                    memcpy(stream->upto, ptr, bytes);
                    stream->upto += bytes;
                    ptr = (char *)ptr + bytes;
                    bytes = 0;
                }
            }
            break;

        case FIXED_TEXT:
            bytes = nmemb * size;
            p = memchr(ptr, '\n', bytes);
            if (p != NULL)
            {
                sz = p - (char *)ptr;
                bytes -= sz + 1;
                if (stream->upto == stream->fbuf)
                {
                    if (sz > stream->lrecl)
                    {
                        sz = stream->lrecl;
                    }
                    begwrite(stream, stream->lrecl);
                    memcpy(dptr, ptr, sz);
                    memset(dptr + sz, ' ', stream->szfbuf - sz);
                    finwrite(stream);
                    stream->bufStartR += sz;
                }
                else
                {
                    if (((stream->upto - stream->fbuf) + sz) > stream->lrecl)
                    {
                        sz = stream->lrecl - (stream->upto - stream->fbuf);
                    }
                    memcpy(stream->upto, ptr, sz);
                    sz += (stream->upto - stream->fbuf);
                    begwrite(stream, stream->lrecl);
                    memcpy(dptr, stream->fbuf, sz);
                    memset(dptr + sz, ' ', stream->lrecl - sz);
                    finwrite(stream);
                    stream->bufStartR += sz;
                    stream->upto = stream->fbuf;
                }
                ptr = (char *)p + 1;
                if (bytes > 0)
                {
                    p = memchr(ptr, '\n', bytes);
                    while (p != NULL)
                    {
                        sz = p - (char *)ptr;
                        bytes -= sz + 1;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        begwrite(stream, stream->lrecl);
                        memcpy(dptr, ptr, sz);
                        memset(dptr + sz, ' ', stream->szfbuf - sz);
                        finwrite(stream);
                        stream->bufStartR += sz;
                        ptr = p + 1;
                        p = memchr(ptr, '\n', bytes);
                    }
                    if (bytes > 0)
                    {
                        sz = bytes;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        memcpy(stream->upto, ptr, sz);
                        stream->upto += sz;
                        bytes = 0;
                    }
                }
            }
            else /* p == NULL */
            {
                if (((stream->upto - stream->fbuf) + bytes) > stream->lrecl)
                {
                    bytes = stream->lrecl - (stream->upto - stream->fbuf);
                }
                memcpy(stream->upto, ptr, bytes);
                stream->upto += bytes;
            }
            break;

        case VARIABLE_TEXT:
            stream->quickText = 0;
            bytes = nmemb * size;
            p = memchr(ptr, '\n', bytes);
            if (p != NULL)
            {
                sz = p - (char *)ptr;
                bytes -= sz + 1;
                if (stream->upto == stream->fbuf)
                {
                    if (sz > stream->lrecl)
                    {
                        sz = stream->lrecl;
                    }
                    begwrite(stream, (sz == 0) ? 5 : sz + 4);
                    if(sz == 0)
                    {
                        dptr[0] = 0;
                        dptr[1] = 5;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        dptr[4] = ' ';
                        finwrite(stream);
                        /* note that the bufStartR needs to reflect
                           just the newline, and not the dummy space
                           we added */
                        stream->bufStartR += 1;
                    }
                    else
                    {
                        dptr[0] = (sz + 4) >> 8;
                        dptr[1] = (sz + 4) & 0xff;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        memcpy(dptr + 4, ptr, sz);
                        finwrite(stream);
                        stream->bufStartR += (sz + 1);
                    }
                }
                else
                {
                    if (((stream->upto - stream->fbuf) + sz) > stream->lrecl)
                    {
                        sz = stream->lrecl - (stream->upto - stream->fbuf);
                    }
                    memcpy(stream->upto, ptr, sz);
                    sz += (stream->upto - stream->fbuf);
                    begwrite(stream, (sz == 0) ? 5 : sz + 4);
                    if(sz == 0)
                    {
                        dptr[0] = 0;
                        dptr[1] = 5;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        dptr[4] = ' ';
                        finwrite(stream);
                        stream->bufStartR += 1;
                    }
                    else
                    {
                        dptr[0] = (sz + 4) >> 8;
                        dptr[1] = (sz + 4) & 0xff;
                        dptr[2] = 0;
                        dptr[3] = 0;
                        memcpy(dptr + 4, stream->fbuf, sz);
                        finwrite(stream);
                        stream->bufStartR += (sz + 1);
                    }
                    stream->upto = stream->fbuf;
                }
                ptr = (char *)p + 1;
                if (bytes > 0)
                {
                    p = memchr(ptr, '\n', bytes);
                    while (p != NULL)
                    {
                        sz = p - (char *)ptr;
                        bytes -= sz + 1;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        begwrite(stream, (sz == 0) ? 5 : sz + 4);
                        if(sz == 0)
                        {
                            dptr[0] = 0;
                            dptr[1] = 5;
                            dptr[2] = 0;
                            dptr[3] = 0;
                            dptr[4] = ' ';
                            finwrite(stream);
                            stream->bufStartR += 1;
                        }
                        else
                        {
                            dptr[0] = (sz + 4) >> 8;
                            dptr[1] = (sz + 4) & 0xff;
                            dptr[2] = 0;
                            dptr[3] = 0;
                            memcpy(dptr + 4, ptr, sz);
                            finwrite(stream);
                            stream->bufStartR += (sz + 1);
                        }
                        ptr = p + 1;
                        p = memchr(ptr, '\n', bytes);
                    }
                    if (bytes > 0)
                    {
                        sz = bytes;
                        if (sz > stream->lrecl)
                        {
                            sz = stream->lrecl;
                        }
                        memcpy(stream->upto, ptr, sz);
                        stream->upto += sz;
                        bytes = 0;
                    }
                }
            }
            else /* p == NULL */
            {
                if (((stream->upto - stream->fbuf) + bytes) > stream->lrecl)
                {
                    bytes = stream->lrecl - (stream->upto - stream->fbuf);
                }
                memcpy(stream->upto, ptr, bytes);
                stream->upto += bytes;
            }
            break;
    }
    return (nmemb);
}

__PDPCLIB_API__ size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t bytes;
    size_t read;
    size_t totalread;
    size_t extra;
    unsigned char *eptr;

    if (stream->quickBin)
    {
        if ((nmemb == 1) && (size == stream->lrecl))
        {
            if (__aread(stream->hfile, &dptr) != 0)
            {
                stream->eofInd = 1;
                stream->quickBin = 0;
                return (0);
            }
            memcpy(ptr, dptr, size);
            stream->bufStartR += size;
            return (1);
        }
        else
        {
            stream->quickBin = 0;
        }
    }
    if (stream->eofInd)
    {
        return (0);
    }

    /* If we have an unget character, then write it into
       the buffer in advance */
    if (stream->ungetCh != -1)
    {
        stream->upto--;
        *stream->upto = stream->ungetCh;
        stream->ungetCh = -1;
    }

    switch (stream->style)
    {
        case FIXED_TEXT:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->bufStartR += (stream->endbuf - stream->fbuf);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }

                eptr = dptr + stream->lrecl - 1;
                while ((*eptr == ' ') && (eptr >= dptr))
                {
                    eptr--;
                }

                read = eptr + 1 - dptr;

                if ((totalread + read) >= bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                    *stream->endbuf++ = '\n';
                }

                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
                if (totalread < bytes)
                {
                    *((char *)ptr + totalread) = '\n';
                    totalread++;
                    stream->bufStartR++;
                }
            }
            return ((size == 0) ? 0 : (totalread / size));
            break;

        case FIXED_BINARY:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->bufStartR += (stream->endbuf - stream->fbuf);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }

                read = stream->lrecl;

                if ((totalread + read) > bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                }

                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
            }
            return ((size == 0) ? 0 : (totalread / size));
            break;

        case VARIABLE_TEXT:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->bufStartR += (stream->endbuf - stream->fbuf);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }

                read = (dptr[0] << 8) | dptr[1];
                read -= 4;
                dptr += 4;
                if ((read == 1) && (dptr[0] == ' '))
                {
                    read = 0;
                }

                if ((totalread + read) >= bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                    *stream->endbuf++ = '\n';
                }

                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
                if (totalread < bytes)
                {
                    *((char *)ptr + totalread) = '\n';
                    totalread++;
                    stream->bufStartR++;
                }
            }
            return ((size == 0) ? 0 : (totalread / size));
            break;

        case VARIABLE_BINARY:
            bytes = nmemb * size;
            read = stream->endbuf - stream->upto;
            if (read > bytes)
            {
                memcpy(ptr, stream->upto, bytes);
                stream->upto += bytes;
                totalread = bytes;
            }
            else
            {
                memcpy(ptr, stream->upto, read);
                stream->bufStartR += (stream->endbuf - stream->fbuf);
                stream->upto = stream->endbuf = stream->fbuf;
                totalread = read;
            }

            while (totalread < bytes)
            {
                if (__aread(stream->hfile, &dptr) != 0)
                {
                    stream->eofInd = 1;
                    break;
                }

                if (!stream->reallyu)
                {
                    read = (dptr[0] << 8) | dptr[1];
                }
                else
                {
                    read = lenread;
                    if (stream->reallyt)
                    {
                        unsigned char *p;

                        /* get rid of any trailing NULs in text mode */
                        p = memchr(dptr, '\0', read);
                        if (p != NULL)
                        {
                            read = p - dptr;
                        }
                    }
                }

                if ((totalread + read) > bytes)
                {
                    extra = (totalread + read) - bytes;
                    read -= extra;
                    memcpy(stream->fbuf, dptr + read, extra);
                    stream->endbuf = stream->fbuf + extra;
                }

                memcpy((char *)ptr + totalread, dptr, read);
                totalread += read;
                stream->bufStartR += read;
            }
            return ((size == 0) ? 0 : (totalread / size));
            break;

        default:
            break;
    }
    return (0);
}

#endif

/*
   filedef dynamically allocates a file (via SVC 99) on MVS-like
   environments.
*/

#if defined(__MVS__)

static struct {
  char len; /* length of request block, always 20 */
  char verb; /* dynamic allocation function requested */
  char flag1;
  char flag2;
  short error_reason; /* returned */
  short info_reason; /* returned */
  void *tu_list; /* list of pointers to text units */
  int reserved;
  char moreflags[4]; /* extraflags */
} rb;

static void *tu_list[10]; /* pointers to text units */

static struct {
  short key; /* key defining what this text unit is */
  short numparms; /* number of parms that follow */
  short parm1_len;
  char parm1[98];
  /* parm2_len etc would theoretically follow, but we
     can't define them, because the length of 98 is probably
     not correct in the first place */
} tu[10];

static void filedef(char *fdddname, char *fnm, int mymode)
{
    memset(&rb, 0x00, sizeof rb);
    rb.len = 20;
    rb.verb = 0x01; /* allocate */
    rb.tu_list = tu_list;

    tu_list[0] = &tu[0];
    tu[0].key = 0x0001; /* ddname */
    tu[0].numparms = 1;
    tu[0].parm1_len = strlen(fdddname);
    strcpy(tu[0].parm1, fdddname);

    tu_list[1] = &tu[1];
    tu[1].key = 0x0002; /* dsname */
    tu[1].numparms = 1;
    tu[1].parm1_len = strlen(fnm);
    strcpy(tu[1].parm1, fnm);

    tu_list[2] = &tu[2];
    tu[2].key = 0x0004; /* disp */
    tu[2].numparms = 1;
    tu[2].parm1_len = 1;
    tu[2].parm1[0] = 0x08; /* SHR */

    tu_list[3] = 0;
    tu_list[4] = 0;
    tu_list[5] = (void *)0x80000000;

    errno = __svc99(&rb);

    /* if we had an error, then for datasets open for write,
       try allocating a new dataset (this will be the normal
       situation - it is abnormal is to find the dataset already
       pre-allocated */
    if (errno)
    {
        /* if open for write */
        if ( mymode )
        {
            tu[2].parm1[0] = 0x04; /* NEW */
            /* Note that the operating system, e.g. PDOS,
               may override the RECFM, e.g. to make everything
               RECFM=U
            */

            /* if binary */
            if (modeType == 5)
            {
                /* F80, which is default */
                /* That seems like a strange default. Regardless,
                   we should be using RECFM=U, BLKSIZE=6233 for
                   output binary files */
            }
            else
            {
                /* V255 */
                tu_list[3] = &tu[3];
                tu[3].key = 0x49; /* RECFM */
                tu[3].numparms = 1;
                tu[3].parm1_len = 1;
                tu[3].parm1[0] = 0x40; /* V */

                tu_list[4] = &tu[4];
                tu[4].key = 0x42; /* LRECL */
                tu[4].numparms = 1;
                tu[4].parm1_len = 2;
                tu[4].parm1[0] = 0; /* LRECL = 255 */
                tu[4].parm1[1] = 255;
            }
        }
        errno = __svc99(&rb);
    }
    if (errno != 0)
    {
        if (rb.error_reason != 0)
        {
            errno = rb.error_reason;
        }
        err = 1;
    }
    return;
}

static void fdclr(char *ddname)
{
    memset(&rb, 0x00, sizeof rb);
    rb.len = 20;
    rb.verb = 0x02; /* unallocate */
    rb.tu_list = tu_list;

    tu_list[0] = &tu[0];
    tu[0].key = 0x0001; /* ddname */
    tu[0].numparms = 1;
    tu[0].parm1_len = strlen(ddname);
    strcpy(tu[0].parm1, ddname);

    tu_list[1] = (void *)0x80000000;

    __svc99(&rb);
    return;
}
#endif


/*
   Following code issues a FILEDEF for CMS
*/

#ifdef __CMS__
static void filedef(char *fdddname, char *fnm, int mymode)
{
    char s202parm [800];

    int code;
    int parm;
    char *fname;
    char *ftype;
    char *fmode;
    char *p;
    int console;

/*
    Skip leading blanks because sometimes people do that in CMS
*/
    while (fnm[0] == ' ') fnm++;

/*
    first parse the file name
*/
    console = 0;
    if( fnm[0] == '*') console = 1;
    while ( NULL != (p = strchr(fnm, '.')) )*p=' '; /* replace . with   */
    fname =  strtok(fnm, " ");
    ftype =  strtok(NULL, " ");
    if (ftype == NULL) ftype = "";
    fmode =  strtok(NULL, " ");
    if (fmode == NULL) fmode = "";


/*
 Now build the SVC 202 string
*/
    memcpy ( &s202parm[0] , "FILEDEF ", 8);
    memcpy ( &s202parm[8] , fdddname, 8);
    if(console)
    {
        memcpy ( &s202parm[16] , "TERMINAL", 8);
        memcpy ( &s202parm[24] , "(       " , 8 );
        memcpy ( &s202parm[32] , "RECFM   " , 8 );
        memcpy ( &s202parm[40] , "V       " , 8 );
        memcpy ( &s202parm[48] , "LRECL   " , 8 );
        memcpy ( &s202parm[56] , "80      " , 8 );
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;
    }
    else
    {
        memcpy ( &s202parm[16] , "DISK    ", 8);
/*
  Clear PARMS area
*/
        memcpy ( &s202parm[24] , "        " , 8);
        memcpy ( &s202parm[32] , "        " , 8);
        if (mymode)
        {
            memcpy ( &s202parm[40] , "A1      " , 8);
            if (fmode[0] != '\0')
            {
                memcpy ( &s202parm[40] , fmode, strlen(fmode));
            }
        }
        else
        {
            memcpy ( &s202parm[40] , "*       " , 8);
            memcpy ( &s202parm[40] , fmode , strlen(fmode) );
        }

        memcpy ( &s202parm[24] , fname ,
                 ( strlen(fname) > 8 ) ? 8 : strlen(fname)  );
        memcpy ( &s202parm[32] , ftype ,
                 ( strlen(ftype) >8 ) ? 8 : strlen(ftype) );
        if ( mymode )
        {
             memcpy ( &s202parm[48] , "(       " , 8 );
             memcpy ( &s202parm[56] , "RECFM   " , 8 );
             memcpy ( &s202parm[64] , "V       " , 8 );
             memcpy ( &s202parm[72] , "LRECL   " , 8 );
             memcpy ( &s202parm[80] , "2000    " , 8 );
             if (modeType == 5)
             {
                 memcpy ( &s202parm[64] , "F       " , 8 );
                 memcpy ( &s202parm[80] , "800     " , 8 );
             }
             s202parm[88]=s202parm[89]=s202parm[90]=s202parm[91]=
                 s202parm[92]=s202parm[93]=s202parm[94]=s202parm[95]=0xff;
        }
        else
        {
             s202parm[48]=s202parm[49]=s202parm[50]=s202parm[51]=
                 s202parm[52]=s202parm[53]=s202parm[54]=s202parm[55]=0xff;
        }
    }
    __SVC202 ( s202parm, &code, &parm );
}

static void fdclr(char *ddname)
{
    char s202parm [800];
    int code;
    int parm;

    /* build the SVC 202 string */
    memcpy( &s202parm[0] , "FILEDEF ", 8);
    memcpy( &s202parm[8] , ddname, 8);
    memcpy( &s202parm[16] , "CLEAR   ", 8);
    memset( &s202parm[24], 0xff, 8);

    __SVC202 ( s202parm, &code, &parm );
    return;
}

/*
   Following code does a rename for CMS
*/

static int cmsrename(const char *old, const char *newnam)
{
    char s202parm[8*8];
    int code;
    int parm;
    char fnm[FILENAME_MAX];
    char *p;
    char *q;

    memset(s202parm, ' ', sizeof s202parm);

    memcpy(&s202parm[0], "RENAME  ", 8);

    strncpy(fnm, old, sizeof fnm);
    fnm[sizeof fnm - 1] = '\0';
    p = fnm;
    while (*p != '\0')
    {
        *p = toupper((unsigned)*p);
        p++;
    }
    p = strchr(fnm, ' ');
    if (p == NULL) p = strchr(fnm, '.');
    if (p == NULL) return (-1);
    *p++ = '\0';
    q = strchr(p, ' ');
    if (q == NULL) q = strchr(p, '.');
    if (q == NULL) q = "A";
    else *q++ = '\0';

    memcpy(&s202parm[8], fnm, (strlen(fnm) > 8 ? 8 : strlen(fnm)));
    memcpy(&s202parm[16], p, (strlen(p) > 8 ? 8 : strlen(p)));
    memcpy(&s202parm[24], q, (strlen(q) > 8 ? 8 : strlen(q)));

    strncpy(fnm, newnam, sizeof fnm);
    fnm[sizeof fnm - 1] = '\0';
    p = fnm;
    while (*p != '\0')
    {
        *p = toupper((unsigned)*p);
        p++;
    }
    p = strchr(fnm, ' ');
    if (p == NULL) p = strchr(fnm, '.');
    if (p == NULL) return (-1);
    *p++ = '\0';
    q = strchr(p, ' ');
    if (q == NULL) q = strchr(p, '.');
    if (q == NULL) q = "A";
    else *q++ = '\0';

    memcpy(&s202parm[32], fnm, (strlen(fnm) > 8 ? 8 : strlen(fnm)));
    memcpy(&s202parm[40], p, (strlen(p) > 8 ? 8 : strlen(p)));
    memcpy(&s202parm[48], q, (strlen(q) > 8 ? 8 : strlen(q)));

    memset(&s202parm[56], 0xff, 8);

    __SVC202(s202parm, &code, &parm);
    return (parm);
}

/*
   Following code does a remove for CMS
*/

static int cmsremove(const char *filename)
{
    char s202parm[5*8];
    int code;
    int parm;
    const char *p;
    const char *q;
    char *f;
    char fnm[FILENAME_MAX];

    strncpy(fnm, filename, sizeof fnm);
    fnm[sizeof fnm - 1] = '\0';
    f = fnm;
    while (*f != '\0')
    {
        *f = toupper((unsigned)*f);
        f++;
    }
    filename = fnm;

    memset(s202parm, ' ', sizeof s202parm);

    /* build the SVC 202 string */
    memcpy( &s202parm[0] , "ERASE   ", 8);

    p = strchr(filename, ' ');
    if (p == NULL)
    {
        p = strchr(filename, '.');
    }
    if (p == NULL)
    {
        memcpy( &s202parm[8] , filename, strlen(filename));
        memset( &s202parm[16], 0xff, 8);
    }
    else
    {
        memcpy( &s202parm[8] , filename, p - filename);
        q = strchr(p + 1, ' ');
        if (q == NULL)
        {
            q = strchr(p + 1, '.');
        }
        if (q == NULL)
        {
            memcpy( &s202parm[16] , p + 1, strlen(p + 1));
            memset( &s202parm[24], 0xff, 8);
        }
        else
        {
            memcpy( &s202parm[16] , p + 1, q - p - 1);
            memcpy( &s202parm[24] , q + 1, strlen(q + 1));
            memset( &s202parm[32], 0xff, 8);
        }
    }

    __SVC202 ( s202parm, &code, &parm );
    return (parm);
}

static char *int_strtok(char *s1, const char *s2)
{
    static char *old = NULL;
    char *p;
    size_t len;
    size_t remain;

    if (s1 != NULL) old = s1;
    if (old == NULL) return (NULL);
    p = old;
    len = strspn(p, s2);
    remain = strlen(p);
    if (remain <= len) { old = NULL; return (NULL); }
    p += len;
    len = strcspn(p, s2);
    remain = strlen(p);
    if (remain <= len) { old = NULL; return (p); }
    *(p + len) = '\0';
    old = p + len + 1;
    return (p);
}

#endif


/*

 The truely cludged piece of code was concocted by Dave Wade

 His erstwhile tutors are probably turning in their graves.

 It is however placed in the Public Domain so that any one
 who wishes to improve is free to do so

*/

static void dblcvt(double num, char cnvtype, size_t nwidth,
            int nprecision, char *result)
{
    double b,round;
    int i,j,exp,pdigits,format;
    char sign, work[125];

    /* save original data & set sign */

    if ( num < 0 )
    {
        b = -num;
        sign = '-';
    }
    else
    {
        b = num;
        sign = ' ';
    }

    /*
      Now scale to get exponent
    */

    exp = 0;
    if( b > 1.0 )
    {
        while ((b >= 10.0) && (exp < 120))
        {
            ++exp;
            b=b / 10.0;
        }
    }
    else if ( b == 0.0 )
    {
        exp=0;
    }
    /* 1.0 will get exp = 0 */
    else if ( b < 1.0 )
    {
        while ((b < 1.0) && (exp > -120))
        {
            --exp;
            b=b*10.0;
        }
    }
    if ((exp <= -120) || (exp >= 120))
    {
        exp = 0;
        b = 0.0;
    }

    /*
      now decide how to print and save in FORMAT.
         -1 => we need leading digits
          0 => print in exp
         +1 => we have digits before dp.
    */

    switch (cnvtype)
    {
        case 'E':
        case 'e':
            format = 0;
            break;
        case 'f':
        case 'F':
            if ( exp >= 0 )
            {
                format = 1;
            }
            else
            {
                format = -1;
            }
            break;
        default:
            /* Style e is used if the exponent from its
               conversion is less than -4 or greater than
               or equal to the precision.
            */
            if ( exp >= 0 )
            {
                if ( nprecision > exp )
                {
                    format=1;
                }
                else
                {
                    format=0;
                }
            }
            else
            {
                /*  if ( nprecision > (-(exp+1) ) ) { */
                if ( exp >= -4)
                {
                    format=-1;
                }
                else
                {
                    format=0;
                }
            }
            break;
    }
    /*
    Now round
    */
    switch (format)
    {
        case 0:    /* we are printing in standard form */
            if (nprecision < DBL_MANT_DIG) /* we need to round */
            {
                j = nprecision;
            }
            else
            {
                j=DBL_MANT_DIG;
            }
            round = 1.0/2.0;
            i = 0;
            while (++i <= j)
            {
                round = round/10.0;
            }
            b = b + round;
            if (b >= 10.0)
            {
                b = b/10.0;
                exp = exp + 1;
            }
            break;

        case 1:      /* we have a number > 1  */
                         /* need to round at the exp + nprecisionth digit */
                if (exp + nprecision < DBL_MANT_DIG) /* we need to round */
                {
                    j = exp + nprecision;
                }
                else
                {
                    j = DBL_MANT_DIG;
                }
                round = 0.5;
                i = 0;
                while (i++ < j)
                {
                    round = round/10;
                }
                b = b + round;
                if (b >= 10.0)
                {
                    b = b/10.0;
                    exp = exp + 1;
                }
                break;

        case -1:   /* we have a number that starts 0.xxxx */
            if (nprecision < DBL_MANT_DIG) /* we need to round */
            {
                j = nprecision + exp + 1;
            }
            else
            {
                j = DBL_MANT_DIG;
            }
            round = 5.0;
            i = 0;
            while (i++ < j)
            {
                round = round/10;
            }
            if (j >= 0)
            {
                b = b + round;
            }
            if (b >= 10.0)
            {
                b = b/10.0;
                exp = exp + 1;
            }
            if (exp >= 0)
            {
                format = 1;
            }
            break;
    }
    /*
       Now extract the requisite number of digits
    */

    if (format==-1)
    {
        /*
             Number < 1.0 so we need to print the "0."
             and the leading zeros...
        */
        result[0]=sign;
        result[1]='0';
        result[2]='.';
        result[3]=0x00;
        while (++exp)
        {
            --nprecision;
            strcat(result,"0");
        }
        i=b;
        --nprecision;
        work[0] = (char)('0' + i % 10);
        work[1] = 0x00;
        strcat(result,work);

        pdigits = nprecision;

        while (pdigits-- > 0)
        {
            b = b - i;
            b = b * 10.0;
            i = b;
            work[0] = (char)('0' + i % 10);
            work[1] = 0x00;
            strcat(result,work);
        }
    }
    /*
       Number >= 1.0 just print the first digit
    */
    else if (format==+1)
    {
        i = b;
        result[0] = sign;
        result[1] = '\0';
        work[0] = (char)('0' + i % 10);
        work[1] = 0x00;
        strcat(result,work);
        nprecision = nprecision + exp;
        pdigits = nprecision ;

        while (pdigits-- > 0)
        {
            if ( ((nprecision-pdigits-1)==exp)  )
            {
                strcat(result,".");
            }
            b = b - i;
            b = b * 10.0;
            i = b;
            work[0] = (char)('0' + i % 10);
            work[1] = 0x00;
            strcat(result,work);
        }
    }
    /*
       printing in standard form
    */
    else
    {
        i = b;
        result[0] = sign;
        result[1] = '\0';
        work[0] = (char)('0' + i % 10);
        work[1] = 0x00;
        strcat(result,work);
        strcat(result,".");

        pdigits = nprecision;

        while (pdigits-- > 0)
        {
            b = b - i;
            b = b * 10.0;
            i = b;
            work[0] = (char)('0' + i % 10);
            work[1] = 0x00;
            strcat(result,work);
        }
    }

    if (format==0)
    { /* exp format - put exp on end */
        work[0] = 'E';
        if ( exp < 0 )
        {
            exp = -exp;
            work[1]= '-';
        }
        else
        {
            work[1]= '+';
        }
        work[2] = (char)('0' + (exp/10) % 10);
        work[3] = (char)('0' + exp % 10);
        work[4] = 0x00;
        strcat(result, work);
    }
    else
    {
        /* get rid of trailing zeros for g specifier */
        if (cnvtype == 'G' || cnvtype == 'g')
        {
            char *p;

            p = strchr(result, '.');
            if (p != NULL)
            {
                p++;
                p = p + strlen(p) - 1;
                while (*p != '.' && *p == '0')
                {
                    *p = '\0';
                    p--;
                }
                if (*p == '.')
                {
                    *p = '\0';
                }
            }
        }
     }
    /* printf(" Final Answer = <%s> fprintf gives=%g\n",
                result,num); */
    /*
     do we need to pad
    */
    if(result[0] == ' ')strcpy(work,result+1); else strcpy(work,result);
    pdigits=nwidth-strlen(work);
    result[0]= 0x00;
    while(pdigits>0)
    {
        strcat(result," ");
        pdigits--;
    }
    strcat(result,work);
    return;
}
