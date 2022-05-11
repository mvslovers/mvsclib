#ifndef __CHAINR_H__
#define __CHAINR_H__

/*--------------------------------------------------------------------
 |  Copyright (c) 2012-2013, Tommy Sprinkle (tommy@tommysprinkle.com)  
 |  Licensed under the NEW BSD Open Source License
  --------------------------------------------------------------------*/

struct sChainRhdr
{
    struct sChainRhdr *next;
    struct sChainRhdr *prev;
    char key;
};

struct sChainRblk
{
    struct sChainRblk *next;
    char *data;
    int free;
};

struct sChainR
{
    char eyeball[4];

    int keyType;
    int keyLen;
    int recSize;
    int allocSize;

    int entryCount;

    struct sChainRblk *firstBlk;
    struct sChainRblk *activeBlk;
    struct sChainRblk *freeBlk;

    struct sChainRhdr *first;
    struct sChainRhdr *last;
};

typedef struct sChainR *CHAINR;

#define chAdd(chainr, key) chainAddFunc(chainr, (void *)key)
#define chainAdd(chainr, key) chainAddFunc(chainr, (void *)key)
#define chainFind(chainr, key) chainFindFunc(chainr, (void *)key)

#define chainNewInt(recsize) chainNewFunc(4, 0, recsize, 4090)
#define chainNewChar(keyln, recsize) chainNewFunc(keyln, 1, recsize, 4090)
#define chainNewBin(keyln, recsize) chainNewFunc(keyln, 2, recsize, 4090)

CHAINR chainNewFunc(int keyLen, int keyType, int recSize, int allocSize);
char *chainAddFunc(CHAINR scr, void *key);

int chainFree(CHAINR scr);
int chainReset(CHAINR scr);

char *chainFirst(CHAINR scr);
char *chainLast(CHAINR scr);
char *chainFindFunc(CHAINR scr, void *key);
int chainCount(CHAINR scr);

#endif
