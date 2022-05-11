/*********************************************************************/
/*                                                                   */
/*  Original Program written by Durand Miller.                       */
/*  Released to the Public Domain                                    */
/*                                                                   */
/*  Modifications by Alica Okano                                     */
/*  Released to the Public Domain as discussed here:                 */
/*  http://creativecommons.org/publicdomain/zero/1.0/                */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
/*                                                                   */
/*  liballoc.h - header file for liballoc.c                          */
/*                                                                   */
/*********************************************************************/

#ifndef LIBALLOC_INCLUDED
#define LIBALLOC_INCLUDED

#include <stddef.h> /* For size_t. */

/* Prefixes malloc, realloc, calloc and free. */
#define LIBALLOC_PREFIX(func) __ ## func
/* Prefixes liballoc hooks. */
#define LIBALLOC_HOOK_PREFIX(func) __liballoc_ ## func

#ifdef __cplusplus
extern "C" {
#endif

/* These are the OS specific functions
 * which need to be implemented on any platform
 * that the library is expected to work on. */

/* This function is supposed to lock the memory data structures.
 * It could be as simple as disabling interrupts or acquiring a spinlock.
 *
 * Return 0 if the lock was acquired successfully.
 * Anything else is failure. */
extern int LIBALLOC_HOOK_PREFIX(lock)();

/* This function unlocks what was previously locked
 * by the lock function.
 * If it disabled interrupts, it enables interrupts,
 * if it had acquired a spinlock, it releases the spinlock...
 *
 * Return 0 if the lock was successfully released. */
extern int LIBALLOC_HOOK_PREFIX(unlock)();

/* This is the hook into the local system which allocates pages.
 * It accepts an integer parameter which is the number of pages required.
 * The page size was set up in the liballoc_init function.
 *
 * Return NULL if the pages were not allocated.
 * Return a pointer to the allocated memory. */
extern void *LIBALLOC_HOOK_PREFIX(alloc)(size_t);

/* This frees previously allocated memory.
 * The void * parameter passed to the function
 * is the exact same value returned
 * from a previous alloc call.
 *
 * The integer value is the number of pages to free.
 *
 * Return 0 if the memory was successfully freed. */
extern int LIBALLOC_HOOK_PREFIX(free)(void *, size_t);
      
/* The standard functions (with the optional prefix). */
extern void *LIBALLOC_PREFIX(malloc)(size_t);
extern void *LIBALLOC_PREFIX(realloc)(void *, size_t);
extern void *LIBALLOC_PREFIX(calloc)(size_t, size_t);
extern void LIBALLOC_PREFIX(free)(void *);

#ifdef __cplusplus
}
#endif

#endif
