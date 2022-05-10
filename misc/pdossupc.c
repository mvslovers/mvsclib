/* written by Paul Edwards */
/* released to the public domain */
/* pdossupc - support routines for pdos */

#include "stddef.h"

#include <pos.h>
#include <support.h>

#if defined(__WATCOMC__)
#define CTYP __cdecl
#else
#define CTYP
#endif

int __open(const char *filename, int mode, int *errind)
{
    int handle;

    if (PosOpenFile(filename, 0, &handle)) *errind = 1;
    else *errind = 0;
    return (handle);
}

int __creat(const char *filename, int mode, int *errind)
{
    int handle;

    if (PosCreatFile(filename, 0, &handle)) *errind = 1;
    else *errind = 0;
    return (handle);
}

int __read(int handle, void *buf, size_t len, int *errind)
{
    size_t readbytes;

    if (PosReadFile(handle, buf, len, &readbytes)) *errind = 1;
    else *errind = 0;
    return (readbytes);
}

int __write(int handle, const void *buf, size_t len, int *errind)
{
    size_t writtenbytes;

    if (PosWriteFile(handle, buf, len, &writtenbytes)) *errind = 1;
    else *errind = 0;
    return (writtenbytes);
}

int __seek(int handle, long offset, int whence)
{
    long dummy;
    return (PosMoveFilePointer(handle, offset, whence, &dummy));
}

void __close(int handle)
{
    PosCloseFile(handle);
    return;
}

void __remove(const char *filename)
{
    PosDeleteFile(filename);
    return;
}

void __rename(const char *old, const char *new)
{
    PosRenameFile(old, new);
    return;
}

#ifdef __32BIT__
/* PDOS-32 uses liballoc with PosVirtualAlloc() and PosVirtualFree(). */
#include "liballoc.h"

int __liballoc_lock()
{
    return (0);
}

int __liballoc_unlock()
{
    return (0);
}

void *__liballoc_alloc(size_t num_pages)
{
    return (PosVirtualAlloc(NULL, num_pages * 0x1000));
}

int __liballoc_free(void *addr, size_t num_pages)
{
    PosVirtualFree(addr, num_pages * 0x1000);

    return (0);
}
#else
void __allocmem(size_t size, void **ptr)
{
    *ptr = PosAllocMem(size, POS_LOC20);
    return;
}

void __freemem(void *ptr)
{
    PosFreeMem(ptr);
    return;
}
#endif

void __exec(char *cmd, void *env)
{
    PosExec(cmd, env);
    return;
}

void __datetime(void *ptr)
{
    int year, month, day, dow;
    unsigned int hour, minute, second, hundredths;
    int *iptr = ptr;

    PosGetSystemDate(&year, &month, &day, &dow);
    iptr[0] = year;
    iptr[1] = month;
    iptr[2] = day;
    PosGetSystemTime(&hour, &minute, &second, &hundredths);
    iptr[3] = hour;
    iptr[4] = minute;
    iptr[5] = second;
    iptr[6] = hundredths;
    PosGetSystemDate(&year, &month, &day, &dow);
    if (day != iptr[2])
    {
        __datetime(ptr);
    }
    return;
}

void __main(void)
{
    return;
}

void CTYP __exita(int retcode)
{
#ifndef PDOS_RET_EXIT
     PosTerminate(retcode);
#endif
    return;
}

#if defined(__WATCOMC__) && defined(__32BIT__)

/* this is invoked by long double manipulations
   in stdio.c and needs to be done properly */

int CTYP _CHP(void)
{
    return (0);
}

/* don't know what these are */

void CTYP cstart_(void) { return; }
void CTYP _argc(void) { return; }
void CTYP argc(void) { return; }
void CTYP _8087(void) { return; }

#endif
