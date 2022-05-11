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
/*  liballoc.c - a small memory allocator                            */
/*                                                                   */
/*********************************************************************/

#include "liballoc.h"

/* C89/C90 does not have uintptr_t. */
#define uintptr_t unsigned long

#define VERSION   "1.1"
/* This is the byte alignment that memory must be allocated on. */
#define ALIGNMENT 16ul

#define ALIGN_TYPE char
/* Alignment information is stored right before the pointer.
 * This is the number of bytes of information stored there. */
#define ALIGN_INFO sizeof(ALIGN_TYPE)*16

/* Define DEBUG or INFO to make liballoc print information. */

#define USE_CASE1
#define USE_CASE2
#define USE_CASE3
#define USE_CASE4
#define USE_CASE5

/* This macro will align the provided pointer upwards. */
#define ALIGN(ptr) \
    if (ALIGNMENT > 1) \
    { \
        uintptr_t diff; \
        ptr = (void *)(((uintptr_t)(ptr)) + ALIGN_INFO); \
        diff = ((uintptr_t)(ptr)) & (ALIGNMENT - 1); \
        if (diff != 0) \
        { \
            diff = ALIGNMENT - diff; \
            (ptr) = (void *)(((uintptr_t)(ptr)) + diff); \
        } \
        *((ALIGN_TYPE*)(((uintptr_t)(ptr)) - ALIGN_INFO)) = (diff \
                                                             + ALIGN_INFO); \
    }                                                           

#define UNALIGN(ptr) \
    if (ALIGNMENT > 1) \
    { \
        uintptr_t diff = *((ALIGN_TYPE*)(((uintptr_t)(ptr)) - ALIGN_INFO)); \
        if (diff < (ALIGNMENT + ALIGN_INFO)) \
        { \
            (ptr) = (void *)(((uintptr_t)(ptr)) - diff); \
        } \
    }

/* Magic values. */
#define LIBALLOC_MAGIC 0xC001C0DE
#define LIBALLOC_DEAD  0xDEADDEAD

#if defined DEBUG || defined INFO
#include <stdio.h>
#include <stdlib.h>

#define FLUSH() fflush(stdout)
#endif

/* A structure found at the top of all system allocated memory blocks.
 * It details the usage of the memory block. */
struct liballoc_major
{
    struct liballoc_major *prev;  /* Linked list information. */
    struct liballoc_major *next;  /* Linked list information. */
    unsigned int pages;           /* The number of pages in the block. */
    unsigned int size;            /* The number of pages in the block. */
    unsigned int usage;           /* The number of bytes used in the block. */
    struct liballoc_minor *first; /* A pointer to the first allocated memory
                                   * in the block. */  
};

/* This is a structure found at the beginning of all sections
 * in a major block which were allocated
 * by a malloc, calloc, or realloc call. */
struct liballoc_minor
{
    struct liballoc_minor *prev;  /* Linked list information. */
    struct liballoc_minor *next;  /* Linked list information. */
    struct liballoc_major *block; /* The owning block.
                                   * A pointer to the major structure. */
    unsigned int magic;           /* A magic number to idenfity correctness. */
    unsigned int size;            /* The size of the memory allocated.
                                   * Could be 1 byte or more. */
    unsigned int req_size;        /* The size of memory requested. */
};

/* The root memory block acquired from the system. */
static struct liballoc_major *l_memRoot = NULL;
/* The major with the most free memory. */
static struct liballoc_major *l_bestBet = NULL;

/* The size of an individual page. Set up in liballoc_init. */
static unsigned int l_pageSize = 4096;
/* The number of pages to request per chunk. Set up in liballoc_init. */
static unsigned int l_pageCount = 16;
/* Running total of allocated memory. */
static unsigned long l_allocated = 0;
/* Running total of used memory. */
static unsigned long l_inuse = 0;

/* Number of warnings encountered. */
static long l_warningCount = 0;
/* Number of actual errors. */
static long l_errorCount = 0;
/* Number of possible overruns. */
static long l_possibleOverruns = 0;

/************   HELPER FUNCTIONS  *******************************/

static void *liballoc_memset(void *s, int c, size_t n)
{
    unsigned int i;
    
    for (i = 0; i < n; i++)
    {
        ((char *)s)[i] = c;
    }
    
    return (s);
}

static void *liballoc_memcpy(void *s1, const void *s2, size_t n)
{
    char *cdest;
    char *csrc;
    unsigned int *ldest = (unsigned int *)s1;
    unsigned int *lsrc = (unsigned int *)s2;

    while (n >= sizeof(unsigned int))
    {
        *ldest++ = *lsrc++;
        n -= sizeof(unsigned int);
    }

    cdest = (char *)ldest;
    csrc = (char *)lsrc;

    while (n > 0)
    {
        *cdest++ = *csrc++;
        n -= 1;
    }

    return (s1);
}

#if defined DEBUG || defined INFO
static void liballoc_dump()
{
#ifdef DEBUG
    struct liballoc_major *maj = l_memRoot;
    struct liballoc_minor *min = NULL;
#endif

    printf("liballoc: ------ Memory data ---------------\n");
    printf("liballoc: System memory allocated: %i bytes\n", l_allocated);
    printf("liballoc: Memory in use (malloc): %i bytes\n", l_inuse);
    printf("liballoc: Warning count: %i\n", l_warningCount);
    printf("liballoc: Error count: %i\n", l_errorCount);
    printf("liballoc: Possible overruns: %i\n", l_possibleOverruns);

#ifdef DEBUG
    while (maj != NULL)
    {
        printf("liballoc: %p: total = %i, used = %i\n",
               maj, maj->size, maj->usage);

        min = maj->first;
        while (min != NULL)
        {
            printf("liballoc:    %p: %i bytes\n",
                   min, min->size);
            min = min->next;
        }

        maj = maj->next;
    }
#endif

    FLUSH();
}
#endif

/****************************************************************/

static struct liballoc_major *allocate_new_page(unsigned int size)
{
    unsigned int st;
    struct liballoc_major *maj;

    /* This is how much space is required. */
    st = size + sizeof(struct liballoc_major);
    st += sizeof(struct liballoc_minor);

    /* Calculates how many pages should be allocated, rounding up. */
    if ((st % l_pageSize) == 0)
    {
        st = st / l_pageSize;
    }
    else
    {
        st = st / l_pageSize + 1;
    }

    /* Ensures it is at least the minimum size. */
    if (st < l_pageCount) st = l_pageCount;

    maj = (struct liballoc_major *)LIBALLOC_HOOK_PREFIX(alloc)(st);

    if (maj == NULL) 
    {
        /* Allocation of pages failed, ran out of memory. */
        l_warningCount += 1;
#if defined DEBUG || defined INFO
        printf("liballoc: WARNING: LIBALLOC_HOOK_PREFIX(alloc)(%i)"
               " returned NULL\n", st);
        FLUSH();
#endif
        return (NULL);
    }

    maj->prev = NULL;
    maj->next = NULL;
    maj->pages = st;
    maj->size = st * l_pageSize;
    maj->usage = sizeof(struct liballoc_major);
    maj->first = NULL;

    l_allocated += maj->size;

#ifdef DEBUG
    printf("liballoc: Resource allocated %p of %i pages (%i bytes)"
           " for %i size.\n",
           maj, st, maj->size, size);

    printf("liballoc: Total memory usage = %i KB\n",
           (int)((l_allocated / 1024)));
    FLUSH();
#endif

    return (maj);
}

void *LIBALLOC_PREFIX(malloc)(size_t req_size)
{
    int startedBet = 0;
    unsigned long bestSize = 0;
    void *p = NULL;
    uintptr_t diff;
    struct liballoc_major *maj;
    struct liballoc_minor *min;
    struct liballoc_minor *new_min;
    unsigned long size = req_size;

    /* For alignment, size is adjusted so there is enough space to align. */
    if (ALIGNMENT > 1)
    {
        size += ALIGNMENT + ALIGN_INFO;
    }

    LIBALLOC_HOOK_PREFIX(lock)();

    if (size == 0)
    {
        l_warningCount += 1;
#if defined DEBUG || defined INFO
        printf("liballoc: WARNING: alloc(0) called from %x\n",
               __builtin_return_address(0));
        FLUSH();
#endif
        LIBALLOC_HOOK_PREFIX(unlock)();
        return (LIBALLOC_PREFIX(malloc)(1));
    }

    if (l_memRoot == NULL)
    {
#if defined DEBUG || defined INFO
#ifdef DEBUG
        printf("liballoc: initialization of liballoc " VERSION "\n");
#endif
        atexit(liballoc_dump);
        FLUSH();
#endif
            
        /* This is the first time liballoc is used. */
        l_memRoot = allocate_new_page(size);
        if (l_memRoot == NULL)
        {
            LIBALLOC_HOOK_PREFIX(unlock)();
#ifdef DEBUG
            printf("liballoc: initial l_memRoot initialization failed\n", p); 
            FLUSH();
#endif
            return (NULL);
        }

#ifdef DEBUG
        printf("liballoc: set up first memory major %x\n", l_memRoot);
        FLUSH();
#endif
    }

#ifdef DEBUG
    printf("liballoc: %x LIBALLOC_PREFIX(malloc)(%i): ",
           __builtin_return_address(0), size);
    FLUSH();
#endif

    /* Searches through every major to find enough space. */
    maj = l_memRoot;
    startedBet = 0;
    
    /* Starts at the best bet... */
    if (l_bestBet != NULL)
    {
        bestSize = l_bestBet->size - (l_bestBet->usage);

        if (bestSize > (size + sizeof(struct liballoc_minor)))
        {
            maj = l_bestBet;
            startedBet = 1;
        }
    }
    
    while (maj != NULL)
    {
        /* Calculates free memory in the block. */
        diff = maj->size - (maj->usage);
        
        if (bestSize < diff)
        {
            /* This one has more memory than our bestBet,
             * so it is remembered. */
            l_bestBet = maj;
            bestSize = diff;
        }
           
#ifdef USE_CASE1    
        /* CASE 1: There is not enough space in this major block. */
        if (diff < (size + sizeof(struct liballoc_minor)))
        {
#ifdef DEBUG
            printf("CASE 1: Insufficient space in block %p\n", maj);
            FLUSH();
#endif
                
            /* If another major block is next to this one,
             * moves onto it. */
            if (maj->next != NULL) 
            {
                maj = maj->next;
                continue;
            }

            if (startedBet == 1)
            {
                /* If the search started at the best bet,
                 * starts from the beginning. */
                maj = l_memRoot;
                startedBet = 0;
                continue;
            }

            /* Creates a new major block next to this one and moves onto it. */
            maj->next = allocate_new_page(size);
            if (maj->next == NULL) break; /* No more memory. */
            maj->next->prev = maj;
            maj = maj->next;

            /* Falls through to CASE 2. */
        }
#endif /* USE_CASE1 */

#ifdef USE_CASE2   
        /* CASE 2: It is a new block. */
        if (maj->first == NULL)
        {
            maj->first = ((struct liballoc_minor *)
                          (((uintptr_t)maj) + sizeof(struct liballoc_major)));

            maj->first->magic = LIBALLOC_MAGIC;
            maj->first->prev = NULL;
            maj->first->next = NULL;
            maj->first->block = maj;
            maj->first->size = size;
            maj->first->req_size = req_size;
            maj->usage += size + sizeof(struct liballoc_minor);

            l_inuse += size;
            
            p = (void *)(((uintptr_t)(maj->first))
                         + sizeof(struct liballoc_minor));
            ALIGN(p);
            
#ifdef DEBUG
            printf("CASE 2: returning %p\n", p); 
            FLUSH();
#endif
            LIBALLOC_HOOK_PREFIX(unlock)();
            return (p);
        }
#endif /* USE_CASE2 */
                
#ifdef USE_CASE3
        /* CASE 3: Block in use and enough space at the start of the block. */
        diff = (uintptr_t)(maj->first);
        diff -= (uintptr_t)maj;
        diff -= sizeof(struct liballoc_major);

        if (diff >= (size + sizeof(struct liballoc_minor)))
        {
            /* Found free space in the front, so it is used. */
            maj->first->prev = ((struct liballoc_minor *)
                                (((uintptr_t)maj)
                                 + sizeof(struct liballoc_major)));
            maj->first->prev->next = maj->first;
            maj->first = maj->first->prev;
                
            maj->first->magic = LIBALLOC_MAGIC;
            maj->first->prev = NULL;
            maj->first->block = maj;
            maj->first->size = size;
            maj->first->req_size = req_size;
            maj->usage += size + sizeof(struct liballoc_minor);

            l_inuse += size;

            p = (void *)(((uintptr_t)(maj->first))
                         + sizeof(struct liballoc_minor));
            ALIGN(p);

#ifdef DEBUG
            printf("CASE 3: returning %p\n", p); 
            FLUSH();
#endif
            LIBALLOC_HOOK_PREFIX(unlock)();
            return (p);
        }
        
#endif /* USE_CASE3 */


#ifdef USE_CASE4
        /* CASE 4: There is enough space in this block. Is it contiguous? */
        min = maj->first;
        
        /* Loops within the block. */
        while (min != NULL)
        {
            /* CASE 4.1: End of minors in a block. Space from last and end? */
            if (min->next == NULL)
            {
                /* The rest of this block is free. Is it big enough? */
                diff = ((uintptr_t)maj) + (maj->size);
                diff -= (uintptr_t)min;
                diff -= sizeof(struct liballoc_minor);
                diff -= min->size; /* Subtracts already used space. */

                if (diff >= (size + sizeof(struct liballoc_minor)))
                {
                    /* There is enough of free space. */
                    min->next = ((struct liballoc_minor *)
                                 (((uintptr_t)min)
                                  + sizeof(struct liballoc_minor)
                                  + (min->size)));
                    
                    min->next->prev = min;
                    min = min->next;
                    min->next = NULL;
                    min->magic = LIBALLOC_MAGIC;
                    min->block = maj;
                    min->size = size;
                    min->req_size = req_size;
                    maj->usage += size + sizeof(struct liballoc_minor);

                    l_inuse += size;
                    
                    p = (void *)(((uintptr_t)min)
                                 + sizeof(struct liballoc_minor));
                    ALIGN(p);

#ifdef DEBUG
                    printf("CASE 4.1: returning %p\n", p); 
                    FLUSH();
#endif
                    LIBALLOC_HOOK_PREFIX(unlock)();
                    return (p);
                }
            }

            /* CASE 4.2: Is there space between two minors? */
            if (min->next != NULL)
            {
                /* Is the difference between here and next big enough? */
                diff = (uintptr_t)(min->next);
                diff -= (uintptr_t)min;
                diff -= sizeof(struct liballoc_minor);
                diff -= min->size; /* Subtracts already used space. */

                if (diff >= (size + sizeof(struct liballoc_minor)))
                {
                    /* There is enough of free space. */
                    new_min = ((struct liballoc_minor *)
                               (((uintptr_t)min)
                                + sizeof(struct liballoc_minor)
                                + (min->size)));
                    
                    new_min->magic = LIBALLOC_MAGIC;
                    new_min->next = min->next;
                    new_min->prev = min;
                    new_min->size = size;
                    new_min->req_size = req_size;
                    new_min->block = maj;
                    min->next->prev = new_min;
                    min->next = new_min;
                    maj->usage += size + sizeof(struct liballoc_minor);
                    
                    l_inuse += size;
                    
                    p = (void *)(((uintptr_t)new_min)
                                 + sizeof(struct liballoc_minor));
                    ALIGN(p);

#ifdef DEBUG
                    printf("CASE 4.2: returning %p\n", p); 
                    FLUSH();
#endif
                    LIBALLOC_HOOK_PREFIX(unlock)();
                    return (p);
                }
            }
            
            min = min->next;
        } /* while (min != NULL) ... */
#endif /* USE_CASE4 */

#ifdef USE_CASE5
        /* CASE 5: Block full! Ensures next block and loops. */
        if (maj->next == NULL) 
        {
#ifdef DEBUG
            printf("CASE 5: block full\n");
            FLUSH();
#endif

            if (startedBet == 1)
            {
                maj = l_memRoot;
                startedBet = 0;
                continue;
            }
                
            /* Allocates a new block. */
            maj->next = allocate_new_page(size);
            if (maj->next == NULL) break; /* Out of memory. */
            maj->next->prev = maj;
        }
#endif /* USE_CASE5 */

        maj = maj->next;
    } /* while (maj != NULL) ... */

#ifdef DEBUG
    printf("All cases exhausted. No memory available.\n");
    FLUSH();
#endif
    
#if defined DEBUG || defined INFO
    printf("liballoc: WARNING: LIBALLOC_PREFIX(malloc)(%i) returning NULL.\n",
           size);
    liballoc_dump();
    FLUSH();
#endif
    LIBALLOC_HOOK_PREFIX(unlock)();
    return (NULL);
}

void LIBALLOC_PREFIX(free)(void *ptr)
{
    struct liballoc_minor *min;
    struct liballoc_major *maj;

    if (ptr == NULL) 
    {
        l_warningCount += 1;
#if defined DEBUG || defined INFO
        printf("liballoc: WARNING: LIBALLOC_PREFIX(free)(NULL)"
               " called from %x\n",
               __builtin_return_address(0));
        FLUSH();
#endif
        return;
    }

    UNALIGN(ptr);

    LIBALLOC_HOOK_PREFIX(lock)();

    min = (struct liballoc_minor *)(((uintptr_t)ptr)
                                    - sizeof(struct liballoc_minor));
    if (min->magic != LIBALLOC_MAGIC) 
    {
        l_errorCount += 1;

        /* Checks for overrun errors. For all bytes of LIBALLOC_MAGIC. */
        if (((min->magic & 0xFFFFFF) == (LIBALLOC_MAGIC & 0xFFFFFF)) || 
            ((min->magic & 0xFFFF) == (LIBALLOC_MAGIC & 0xFFFF)) || 
            ((min->magic & 0xFF) == (LIBALLOC_MAGIC & 0xFF)))
        {
            l_possibleOverruns += 1;
#if defined DEBUG || defined INFO
            printf("liballoc: ERROR: Possible 1-3 byte overrun"
                   " for magic %x != %x\n",
                   min->magic, LIBALLOC_MAGIC );
            FLUSH();
#endif
        }         
                        
        if (min->magic == LIBALLOC_DEAD)
        {
#if defined DEBUG || defined INFO
            printf("liballoc: ERROR: multiple LIBALLOC_PREFIX(free)() attempts"
                   " on %x from %x.\n",
                   ptr, __builtin_return_address(0));
            FLUSH();
#endif
        }
        else
        {
#if defined DEBUG || defined INFO
            printf("liballoc: ERROR: Bad LIBALLOC_PREFIX(free)(%x)"
                   " called from %x\n",
                   ptr, __builtin_return_address(0));
            FLUSH();
#endif
        }

        LIBALLOC_HOOK_PREFIX(unlock)();
        return;
    }

#ifdef DEBUG
    printf("liballoc: %x LIBALLOC_PREFIX(free)(%x): ",
           __builtin_return_address(0), ptr);
    FLUSH();
#endif
    
    maj = min->block;

    l_inuse -= min->size;

    maj->usage -= min->size + sizeof(struct liballoc_minor);
    min->magic = LIBALLOC_DEAD;

    if (min->next != NULL) min->next->prev = min->prev;
    if (min->prev != NULL) min->prev->next = min->next;
    
    if (min->prev == NULL)
    {
        /* Might empty the block as this was the first minor. */
        maj->first = min->next;
    }

    /* Logic for handling majors. */
    if (maj->first == NULL)
    {
        /* Block completely unused. */
        if (l_memRoot == maj) l_memRoot = maj->next;
        if (l_bestBet == maj) l_bestBet = NULL;
        if (maj->prev != NULL) maj->prev->next = maj->next;
        if (maj->next != NULL) maj->next->prev = maj->prev;
        l_allocated -= maj->size;

        LIBALLOC_HOOK_PREFIX(free)(maj, maj->pages);
    }
    else
    {
        if (l_bestBet != NULL)
        {
            int bestSize = l_bestBet->size - (l_bestBet->usage);
            int majSize = maj->size - (maj->usage);

            if (majSize > bestSize) l_bestBet = maj;
        }
    }

#ifdef DEBUG
    printf("OK\n");
    FLUSH();
#endif
    
    LIBALLOC_HOOK_PREFIX(unlock)();
}

void *LIBALLOC_PREFIX(calloc)(size_t nobj, size_t size)
{
   int real_size;
   void *p;

   real_size = nobj * size;
   
   p = LIBALLOC_PREFIX(malloc)(real_size);

   liballoc_memset(p, 0, real_size);

   return (p);
}

void *LIBALLOC_PREFIX(realloc)(void *p, size_t size)
{
    void *ptr;
    struct liballoc_minor *min;
    unsigned int real_size;
    
    /* In the case size is 0, frees the old and returns NULL. */
    if (size == 0)
    {
        LIBALLOC_PREFIX(free)(p);
        return (NULL);
    }

    /* In the case of a NULL pointer, returns a simple malloc. */
    if (p == NULL) return LIBALLOC_PREFIX(malloc)(size);

    /* Unaligns the pointer if required. */
    ptr = p;
    UNALIGN(ptr);

    LIBALLOC_HOOK_PREFIX(lock)();

    min = (struct liballoc_minor *)(((uintptr_t)ptr)
                                    - sizeof(struct liballoc_minor));
    
    /* Ensures it is a valid structure. */
    if (min->magic != LIBALLOC_MAGIC) 
    {
        l_errorCount += 1;

        /* Checks for overrun errors. For all bytes of LIBALLOC_MAGIC. */
        if (((min->magic & 0xFFFFFF) == (LIBALLOC_MAGIC & 0xFFFFFF)) || 
            ((min->magic & 0xFFFF) == (LIBALLOC_MAGIC & 0xFFFF)) || 
            ((min->magic & 0xFF) == (LIBALLOC_MAGIC & 0xFF)))
        {
            l_possibleOverruns += 1;
#if defined DEBUG || defined INFO
            printf("liballoc: ERROR: Possible 1-3 byte overrun"
                   " for magic %x != %x\n",
                   min->magic, LIBALLOC_MAGIC );
            FLUSH();
#endif
        }
       
        if (min->magic == LIBALLOC_DEAD)
        {
#if defined DEBUG || defined INFO
            printf("liballoc: ERROR: multiple LIBALLOC_PREFIX(free)() attempts"
                   " on %p from %x.\n",
                   ptr, __builtin_return_address(0));
            FLUSH();
#endif
        }
        else
        {
#if defined DEBUG || defined INFO
            printf("liballoc: ERROR: Bad LIBALLOC_PREFIX(free)(%p)"
                   " called from %x\n",
                   ptr, __builtin_return_address(0));
            FLUSH();
#endif
        }

        LIBALLOC_HOOK_PREFIX(unlock)();
        return (NULL);
    }   
    
    real_size = min->req_size;

    if (real_size >= size) 
    {
        min->req_size = size;
        LIBALLOC_HOOK_PREFIX(unlock)();
        return (p);
    }

    LIBALLOC_HOOK_PREFIX(unlock)();

    /* Reallocates to a bigger block than the one provided. */
    ptr = LIBALLOC_PREFIX(malloc)(size);
    if (ptr == NULL)
    {
        /* New block was not allocated, so the old one must be preserved. */
        return (NULL);
    }
    liballoc_memcpy(ptr, p, real_size);
    LIBALLOC_PREFIX(free)(p);

    return (ptr);
}

