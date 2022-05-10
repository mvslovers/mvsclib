#ifndef __ALLOC_H__
#define __ALLOC_H__

/*-----------------------------------------------------------------
 |  Copyright (c) 2012-, Tommy Sprinkle (tommy@tommysprinkle.com)  
 |  Licensed under the NEW BSD Open Source License
  -----------------------------------------------------------------*/

char *dynAlloc(char *dsn, char *rtddn, char *volser, char *device);
char *dynFree(char *ddn);

#endif
