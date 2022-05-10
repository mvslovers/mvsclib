/*********************************************************************/
/*                                                                   */
/*  This Program Written by Paul Edwards.                            */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  start.c - startup/termination code                               */
/*                                                                   */
/*********************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stddef.h"

#if USE_MEMMGR
#include "__memmgr.h"
#endif

extern FILE *__userFiles[__NFILE];

#define MAXPARMS 50 /* maximum number of arguments we can handle */

#ifdef __OS2__
#define INCL_DOS
#include <os2.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#if defined(__WATCOMC__)
#define CTYP __cdecl
#else
#define CTYP
#endif

#if defined(__PDOS386__)
/* Used for PDOS itself to avoid API calls when starting. */
int __minstart = 0;
#endif

#ifdef __MSDOS__
/* Must be unsigned as it is used for array index */
extern unsigned char *__envptr;
extern unsigned short __osver;
#endif

#ifdef __VSE__
#undef __CMS__
#undef __MVS__
#endif

#if defined(__MVS__) || defined(__CMS__) || defined(__VSE__)
int __tso = 0; /* is this a TSO environment? */
extern int __doperm; /* are we doing the permanent datasets? */
int __upsi = 0; /* UPSI switches for VSE */
#endif

int main(int argc, char **argv);

void __exit(int status);
void CTYP __exita(int status);

#if !defined(__MVS__) && !defined(__CMS__) && !defined(__VSE__)
static char buffer1[BUFSIZ + 8];
static char buffer2[BUFSIZ + 8];
static char buffer3[BUFSIZ + 8];
#endif

#if defined(__PDOS386__)
#include <support.h>
#include <pos.h>
unsigned char *__envptr;
#endif

#if USE_MEMMGR
extern void *__lastsup; /* last thing supplied to memmgr */
#endif

char **__eplist;
char *__plist;

#ifdef __WIN32__
/* Not sure what _startupinfo is. */
typedef int _startupinfo;

__PDPCLIB_API__ int __getmainargs(int *_Argc,
                                  char ***_Argv,
                                  char ***_Env,
                                  int _DoWildCard,
                                  _startupinfo *_StartInfo)
{
    char *p;
    int x;
    int argc;
    static char *argv[MAXPARMS + 1];
    static char *env[] = {NULL};

    p = GetCommandLine();

    argv[0] = p;
    p = strchr(p, ' ');
    if (p == NULL)
    {
        p = "";
    }
    else
    {
        *p = '\0';
        p++;
    }

    while (*p == ' ')
    {
        p++;
    }
    if (*p == '\0')
    {
        argv[1] = NULL;
        argc = 1;
    }
    else
    {
        for (x = 1; x < MAXPARMS; )
        {
            char srch = ' ';

            if (*p == '"')
            {
                p++;
                srch = '"';
            }
            argv[x] = p;
            x++;
            p = strchr(p, srch);
            if (p == NULL)
            {
                break;
            }
            else
            {
                *p = '\0';
                p++;
                while (*p == ' ') p++;
                if (*p == '\0') break; /* strip trailing blanks */
            }
        }
        argv[x] = NULL;
        argc = x;
    }

    *_Argc = argc;
    *_Argv = argv;
    *_Env = env;
}
#endif

#if defined(__CMS__)
int __start(char *plist, char *pgmname, char **eplist)
#elif defined(__VSE__)
int __start(char *p, char *pgmname, char *ep)
#elif defined(__MVS__)
int __start(char *p, char *pgmname, int tso)
#elif defined(__gnu_linux__)
int __start(int argc, char **argv)
#else
__PDPCLIB_API__ int CTYP __start(char *p)
#endif
{
#ifdef __CMS__
    char *p;
#endif
    int x;
#if !defined(__gnu_linux__)
    int argc;
    static char *argv[MAXPARMS + 1];
#endif
    int rc;
#ifdef __OS2__
    ULONG maxFH;
    LONG reqFH;
#endif
#ifdef __MSDOS__
    unsigned char *env;
#endif
#if defined(__MVS__) || defined(__CMS__) || defined(__VSE__)
    int parmLen;
    int progLen;
    char parmbuf[310]; /* z/VSE can have a PARM up to 300 characters */
#endif

#if !defined(__MVS__) && !defined(__CMS__) && !defined(__VSE__)
#ifdef __WIN32__
    __stdin->hfile = GetStdHandle(STD_INPUT_HANDLE);
    __stdout->hfile = GetStdHandle(STD_OUTPUT_HANDLE);
    __stderr->hfile = GetStdHandle(STD_ERROR_HANDLE);
#else
    __stdin->hfile = 0;
    __stdout->hfile = 1;
    __stderr->hfile = 2;
#endif

    __stdin->quickBin = 0;
    __stdin->quickText = 0;
    __stdin->textMode = 1;
    __stdin->intFno = 0;
    __stdin->bufStartR = 0;
    __stdin->justseeked = 0;
    __stdin->bufTech = _IOLBF;
    __stdin->intBuffer = buffer1;
    __stdin->fbuf = __stdin->intBuffer + 2;
    *__stdin->fbuf++ = '\0';
    *__stdin->fbuf++ = '\0';
    __stdin->szfbuf = BUFSIZ;
    __stdin->endbuf = __stdin->fbuf + __stdin->szfbuf;
    *__stdin->endbuf = '\n';
    __stdin->noNl = 0;
    __stdin->upto = __stdin->endbuf;
    __stdin->bufStartR = -__stdin->szfbuf;
    __stdin->mode = __READ_MODE;
    __stdin->ungetCh = -1;
    __stdin->update = 0;
    __stdin->theirBuffer = 0;
    __stdin->permfile = 1;
    __stdin->isopen = 1;

    __stdout->quickBin = 0;
    __stdout->quickText = 0;
    __stdout->textMode = 1;
    __stdout->bufTech = _IOLBF;
    __stdout->intBuffer = buffer2;
    __stdout->fbuf = __stdout->intBuffer;
    *__stdout->fbuf++ = '\0';
    *__stdout->fbuf++ = '\0';
    __stdout->szfbuf = BUFSIZ;
    __stdout->endbuf = __stdout->fbuf + __stdout->szfbuf;
    *__stdout->endbuf = '\n';
    __stdout->noNl = 0;
    __stdout->upto = __stdout->fbuf;
    __stdout->bufStartR = 0;
    __stdout->justseeked = 0;
    __stdout->mode = __WRITE_MODE;
    __stdout->update = 0;
    __stdout->theirBuffer = 0;
    __stdout->permfile = 1;
    __stdout->isopen = 1;

    __stderr->quickBin = 0;
    __stderr->quickText = 0;
    __stderr->textMode = 1;
    __stderr->bufTech = _IOLBF;
    __stderr->intBuffer = buffer3;
    __stderr->fbuf = __stderr->intBuffer;
    *__stderr->fbuf++ = '\0';
    *__stderr->fbuf++ = '\0';
    __stderr->szfbuf = BUFSIZ;
    __stderr->endbuf = __stderr->fbuf + __stderr->szfbuf;
    *__stderr->endbuf = '\n';
    __stderr->noNl = 0;
    __stderr->upto = __stderr->fbuf;
    __stderr->bufStartR = 0;
    __stderr->justseeked = 0;
    __stderr->mode = __WRITE_MODE;
    __stderr->update = 0;
    __stderr->theirBuffer = 0;
    __stderr->permfile = 1;
    __stderr->isopen = 1;
#else
    int dyna_sysprint = 0;
    int dyna_systerm = 0;
    int dyna_sysin = 0;
#if defined(__CMS__)
/*
  This code checks to see if DDs exist for SYSIN, SYSPRINT & SYSTERM
  if not it issues FD to the terminal
*/
    char s202parm [800]; /* svc 202 buffer */
    int code;
    int parm;
    int ret;
    int have_sysparm;

/*
 Now build the SVC 202 string for sysprint
*/
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSPRINT", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
/*
  and issue the SVC
*/
    ret = __SVC202 ( s202parm, &code, &parm );
    if (ret == 24)
    { /* we need to issue filedef */
        memcpy ( &s202parm[16] , "TERM    ", 8);
        memcpy ( &s202parm[24] , "(       ", 8);
        memcpy ( &s202parm[32] , "LRECL   ", 8);
        memcpy ( &s202parm[40] , "80      ", 8);
        memcpy ( &s202parm[48] , "RECFM   ", 8);
        memcpy ( &s202parm[56] , "F       ", 8);
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;

        ret = __SVC202 ( s202parm, &code, &parm );
        dyna_sysprint = 1;
    }

/*
 Now build the SVC 202 string for systerm
*/
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSTERM ", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
/*
  and issue the SVC
*/
    ret = __SVC202 ( s202parm, &code, &parm );
    if (ret == 24)
    { /* we need to issue filedef */
        memcpy ( &s202parm[16] , "TERM    ", 8);
        memcpy ( &s202parm[24] , "(       ", 8);
        memcpy ( &s202parm[32] , "LRECL   ", 8);
        memcpy ( &s202parm[40] , "80      ", 8);
        memcpy ( &s202parm[48] , "RECFM   ", 8);
        memcpy ( &s202parm[56] , "F       ", 8);
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;

        ret = __SVC202 ( s202parm, &code, &parm );
        dyna_systerm = 1;
    }

/*
 Now build the SVC 202 string for sysin
*/
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSIN   ", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
/*
  and issue the SVC
*/
    ret = __SVC202 ( s202parm, &code, &parm );

    if (ret == 24)
    { /* we need to issue filedef */
        memcpy ( &s202parm[16] , "TERM    ", 8);
        memcpy ( &s202parm[24] , "(       ", 8);
        memcpy ( &s202parm[32] , "LRECL   ", 8);
        memcpy ( &s202parm[40] , "80      ", 8);
        memcpy ( &s202parm[48] , "RECFM   ", 8);
        memcpy ( &s202parm[56] , "F       ", 8);
        s202parm[64]=s202parm[65]=s202parm[66]=s202parm[67]=
            s202parm[68]=s202parm[69]=s202parm[70]=s202parm[71]=0xff;

        ret = __SVC202 ( s202parm, &code, &parm );
        dyna_sysin = 1;
    }

#endif
#if USE_MEMMGR
    memmgrDefaults(&__memmgr);
    memmgrInit(&__memmgr);
#endif
#if 0 /* MUSIC */
    /* switch on lowercasing of input */
    /* normal MUSIC default is to uppercase, and it's probably
       better to let the user control that with the /TEXT LC
       command instead */
    __textlc();
#endif
#if defined(__MVS__)
    /* need to know if this is a TSO environment straight away
       because it determines how the permanent files will be
       opened */
    parmLen = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
#if 1 /* traditional way of checking to see if it is TSO */
    if ((parmLen > 0) && (p[2] == 0))     /* assume TSO */
    {
        __tso = 1;
    }
#else
    __tso = (tso != 0); /* even "CALL" is considered to be TSO */
#endif

#endif /* MVS */
    __doperm = 1;
    __stdout = fopen("dd:SYSPRINT", "w");
    if (__stdout == NULL)
    {
        __exita(EXIT_FAILURE);
    }
    __stdout->dynal = dyna_sysprint;

    __stderr = fopen("dd:SYSTERM", "w");
    if (__stderr == NULL)
    {
        printf("SYSTERM DD not defined\n");
        fclose(__stdout);
        __exita(EXIT_FAILURE);
    }
    __stderr->dynal = dyna_systerm;

    __stdin = fopen("dd:SYSIN", "r");
    if (__stdin == NULL)
    {
        fprintf(__stderr, "SYSIN DD not defined\n");
        fclose(__stdout);
        fclose(__stderr);
        __exita(EXIT_FAILURE);
    }
    __stdin->dynal = dyna_sysin;
    __doperm = 0;
#if defined(__CMS__)
    __eplist = eplist;
    __plist = plist;

    if (plist[0] == '\xff')  /* are we at the fence already? */
    {
        p = plist;   /* yes, this is also the start of the plist */
    }
    else
    {
        p = plist + 8; /* no, so jump past the command name */
    }

    /* Now build the SVC 202 string for sysparm */
    memcpy ( &s202parm[0] ,  "FILEDEF ", 8);
    memcpy ( &s202parm[8] ,  "SYSPARM ", 8);
    memcpy ( &s202parm[16] , "(       ", 8);
    memcpy ( &s202parm[24] , "NOCHANGE", 8);
    s202parm[32]=s202parm[33]=s202parm[34]=s202parm[35]=
        s202parm[36]=s202parm[37]=s202parm[38]=s202parm[39]=0xff;
    /* and issue the SVC */
    ret = __SVC202 ( s202parm, &code, &parm );

    have_sysparm = (ret != 24);


    /* if no parameters are provided, the tokenized
       plist will start with x'ff'. However, if they
       have provided a SYSPARM, then we'll use that
       as the parameter. But only if they haven't
       provided any parameters! If they have provided
       parameters then we instead lowercase everything
       and go to special processing (useful when in
       an EXEC with CONTROL MSG etc). */

    /* No parameters */
    if (p[0] == 0xff)
    {
        parmLen = 0;

        if (have_sysparm)
        {
            FILE *pf;

            /* have a parameter file - let's use it */
            pf = fopen("dd:SYSPARM", "r");
            if (pf != NULL)
            {
                fgets(parmbuf + 2, sizeof parmbuf - 2, pf);
                fclose(pf);
                p = strchr(parmbuf + 2, '\n');
                if (p != NULL)
                {
                    *p = '\0';
                }
                parmLen = strlen(parmbuf + 2);
            }
        }
    }
    /* If there is no EPLIST, or there is a SYSPARM so
       they are invoking special processing, then we
       will be using the PLIST only. */
    else if ((eplist == NULL) || have_sysparm)
    {
        /* copy across the tokenized plist, which
           consists of 8 character chunks, space-padded,
           and terminated by x'ff'. Note that the first
           2 characters of parmbuf are reserved for an
           (unused) length, so we must skip them */
        for (x = 0; x < sizeof parmbuf / 9 - 1; x++)
        {
            if (p[x * 8] == 0xff) break;
            memcpy(parmbuf + 2 + x * 9, p + x * 8, 8);
            parmbuf[2 + x * 9 + 8] = ' ';
        }
        parmbuf[2 + x * 9] = '\0';
        parmLen = strlen(parmbuf + 2);

        /* even though we have a SYSPARM, we don't use it,
           we just use it as a signal to do some serious
           underscore searching! */
        if (have_sysparm)
        {
            char *q;
            char *r;
            char *lock;
            int cnt = 0;
            int c;
            int shift = 0;
            int rev = 0; /* reverse logic */

            q = parmbuf + 2;
            r = q;
            lock = q;

            /* reverse the case switching when _+ is specified
               as the first parameter */
            if (memcmp(r, "_+", 2) == 0)
            {
                rev = 1;
                cnt += 2;
                r += 2;
            }
            while (*r != '\0')
            {
                cnt++;
                if (rev)
                {
                    c = toupper((unsigned char)*r);
                }
                else
                {
                    c = tolower((unsigned char)*r);
                }
                if (shift && (c != ' '))
                {
                    if (rev)
                    {
                        c = tolower((unsigned char)*r);
                    }
                    else
                    {
                        c = toupper((unsigned char)*r);
                    }
                    shift = 0;
                }
                if (c == '_')
                {
                    shift = 1;
                }
                /* if we've reached the inter-parameter space, then
                   collapse it - a space requires a shift */
                else if (cnt == 9)
                {
                    while (q > lock)
                    {
                        q--;
                        if (*q != ' ')
                        {
                            q++;
                            lock = q;
                            break;
                        }
                    }
                    cnt = 0;
                    if (shift)
                    {
                        *q++ = ' ';
                        shift = 0;
                    }
                }
                else if (c != ' ')
                {
                    *q++ = c;
                }
                r++;
            }
            *q = '\0';
            parmLen = strlen(parmbuf + 2);
        }
    }
    /* else, we have an eplist, and no sysparm, so use that */
    else
    {
        parmLen = eplist[2] - eplist[1];
        /* 2 bytes reserved for an unused length, 1 byte for NUL */
        if (parmLen >= sizeof parmbuf - 2)
        {
            parmLen = sizeof parmbuf - 1 - 2;
        }
        memcpy(parmbuf + 2, eplist[1], parmLen);
    }
#elif defined(__VSE__)
    __upsi = pgmname[9]; /* we shouldn't really clump this */

    if (ep != NULL)
    {
        ep = *(char **)ep;
    }
    /* The ep only has a genuine value if the top bit is set */
    if (((unsigned int)ep & 0x80000000) != 0)
    {
        /* it is a 24-bit address */
        ep = (char *)((unsigned int)ep & 0x00ffffff);
        parmLen = *(short *)ep;
        memcpy(parmbuf + 2, ep + 2, parmLen);
    }
    /* if no parm, use SYSPARM instead */
    else if (p[0] != 0)
    {
        /* in the special case of a "?", inspect the UPSI switches */
        if ((p[0] == 1) && (p[1] == '?'))
        {
            /* user is required to set all switches to 0. All
               are reserved, except for the first one, which
               says that the parameter will be read from SYSINPT */
            if (__upsi & 0x80)
            {
                fgets(parmbuf + 2, sizeof parmbuf - 2, __stdin);
                p = strchr(parmbuf + 2, '\n');
                if (p != NULL)
                {
                    *p = '\0';
                }
                parmLen = strlen(parmbuf + 2);
            }
            else
            {
                parmLen = 0;
            }
        }
        /* for all other parameter values, just use as-is */
        else
        {
            parmLen = p[0];
            memcpy(parmbuf + 2, p + 1, parmLen);
        }

    }
    /* otherwise there is no parm */
    else
    {
        parmLen = 0;
    }
#else /* MVS etc */
    parmLen = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
    if (parmLen >= sizeof parmbuf - 2)
    {
        parmLen = sizeof parmbuf - 1 - 2;
    }
    /* We copy the parameter into our own area because
       the caller hasn't necessarily allocated room for
       a terminating NUL, nor is it necessarily correct
       to clobber the caller's area with NULs. */
    memcpy(parmbuf, p, parmLen + 2);
#endif
    p = parmbuf;
#ifdef __MVS__
    if (__tso)
#else
    if (0)
#endif
    {
        progLen = ((unsigned int)p[2] << 8) | (unsigned int)p[3];
        parmLen -= (progLen + 4);
        argv[0] = p + 4;
        p += (progLen + 4);
        if (parmLen > 0)
        {
            *(p - 1) = '\0';
        }
        else
        {
            *p = '\0';
        }
        p[parmLen] = '\0';
    }
    else         /* batch or tso "call" */
    {
        progLen = 0;
        p += 2;
        argv[0] = pgmname;
        pgmname[8] = '\0';
        pgmname = strchr(pgmname, ' ');
        if (pgmname != NULL)
        {
            *pgmname = '\0';
        }
        if (parmLen > 0)
        {
            p[parmLen] = '\0';
        }
        else
        {
            p = "";
        }
    }
#endif /* defined(__MVS__) || defined(__CMS__) || defined(__VSE__) */

    for (x=0; x < __NFILE; x++)
    {
        __userFiles[x] = NULL;
    }

#ifdef __PDPCLIB_DLL
    return (0);
#endif

#if defined(__PDOS386__)
    /* PDOS-32 uses an API call returning the full command line string. */
    if (!__minstart)
    {
        p = PosGetCommandLine();
        __envptr = PosGetEnvBlock();
    }
    else
    {
        /* PDOS itself is starting so no API calls should be used. */
        p = "";
        __envptr = NULL;
    }
#endif

#ifdef __WIN32__
    p = GetCommandLine();
#endif

#ifdef __OS2__
    reqFH = 0;
    DosSetRelMaxFH(&reqFH, &maxFH);
    if (maxFH < (FOPEN_MAX + 10))
    {
        reqFH = FOPEN_MAX - maxFH + 10;
        DosSetRelMaxFH(&reqFH, &maxFH);
    }
#endif
#ifdef __OS2__
    argv[0] = p;
    p += strlen(p) + 1;
#endif
#if defined(__WIN32__) || defined(__PDOS386__)
    /* Windows and PDOS-32 get the full command line string. */
    argv[0] = p;
    p = strchr(p, ' ');
    if (p == NULL)
    {
        p = "";
    }
    else
    {
        *p = '\0';
        p++;
    }
#elif defined(__MSDOS__)
    argv[0] = "";

    if(__osver > 0x300)
    {
        env=__envptr;
        while (1)
        {
            if (*env++ == '\0' && *env++ == '\0')
            {
                if (*(unsigned short *)env != 0)
                {
                    argv[0] = (char *)env + 2;
                }
                break;
            }
        }
    }
    p = p + 0x80;
    p[*p + 1] = '\0';
    p++;
#endif
#if !defined(__gnu_linux__)
    while (*p == ' ')
    {
        p++;
    }
    if (*p == '\0')
    {
        argv[1] = NULL;
        argc = 1;
    }
    else
    {
        for (x = 1; x < MAXPARMS; )
        {
            char srch = ' ';

            if (*p == '"')
            {
                p++;
                srch = '"';
            }
            argv[x] = p;
            x++;
            p = strchr(p, srch);
            if (p == NULL)
            {
                break;
            }
            else
            {
                *p = '\0';
                p++;
                while (*p == ' ') p++;
                if (*p == '\0') break; /* strip trailing blanks */
            }
        }
        argv[x] = NULL;
        argc = x;
    }
#endif
#ifdef PDOS_MAIN_ENTRY
    *i1 = argc;
    *i2 = (int)argv;
    return (0);
#elif defined(__PDPCLIB_DLL)
    return (0);
#else
    rc = main(argc, argv);

    __exit(rc);
    return (rc);
#endif
}

void _exit(int status);
void _cexit(void);
void _c_exit(void);

void __exit(int status)
{
    /* Complete C library termination and exit with error code. */
    _cexit();

#ifdef __WIN32__
    ExitProcess(status);
#else
    __exita(status);
#endif
}

__PDPCLIB_API__ void _exit(int status)
{
    /* Quick C library termination and exit with error code.. */
    _c_exit();

#ifdef __WIN32__
    ExitProcess(status);
#else
    __exita(status);
#endif
}

__PDPCLIB_API__ void _cexit(void)
{
    /* Complete C library termination. */
    _c_exit();
}

__PDPCLIB_API__ void _c_exit(void)
{
    /* Quick C library termination. */
    int x;

#if 0
    for (x = __NATEXIT - 1; x >= 0; x--)
    {
        if (__userExit[x] != 0)
        {
            (__userExit[x])();
        }
    }
#endif

    for (x = 0; x < __NFILE; x++)
    {
        if (__userFiles[x] != NULL)
        {
#if defined(__VSE__)
            /* this should be closed after the rest of the user files */
            if (__userFiles[x] != __stdpch)
#endif
            fclose(__userFiles[x]);
        }
    }

#if defined(__VSE__)
    if (__stdpch != NULL) fclose(__stdpch);
#endif

    if (__stdout != NULL) fflush(__stdout);
    if (__stderr != NULL) fflush(__stderr);
#if defined(__MVS__) || defined(__CMS__) || defined(__VSE__)
    if (__stdin != NULL) fclose(__stdin);
    if (__stdout != NULL) fclose(__stdout);
    if (__stderr != NULL) fclose(__stderr);
#endif


#if USE_MEMMGR
    memmgrTerm(&__memmgr);

/* release memory for most circumstances, although a
   better solution will be required eventually */
#if defined(__MVS__) || defined(__CMS__) || defined(__VSE__)
    if (__lastsup != NULL)
    {
        __freem(__lastsup);
    }
#endif
#endif /* USE_MEMMGR */
}

#ifdef __PDPCLIB_DLL
BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    __start(0);

    return (TRUE);
}
#endif

#ifdef __WIN32__
/* Windows extensions. */
static int _fmode;
__PDPCLIB_API__ int *__p__fmode(void)
{
    /* Not sure what should this function do. */
    return (&_fmode);
}

static char *_environ[] = {NULL};
static char **_environ_ptr = _environ;
__PDPCLIB_API__ char ***__p__environ(void)
{
    /* Not sure what should this function do. */
    return (&_environ_ptr);
}

__PDPCLIB_API__ void __set_app_type(int at)
{
    /* Not sure what should this function do. */
    ;
}

__PDPCLIB_API__ int _setmode(int fd, int mode)
{
    /* Should change mode of file descriptor (fd)
     * to mode (binary, text, Unicode...)
     * and return the previous mode.
     * We do not have _fileno() to convert FILE *
     * to int and _fileno() can be implemented
     * as macro accesing FILE internals...,
     * so this function is just a dummy. */
    return (0);
}

__PDPCLIB_API__ void (*_onexit(void (*func)(void)))(void)
{
    if (atexit(func)) return (NULL);
    return (func);
}
#endif
