#ifndef __SVC_H__
#define __SVC_H__

/*-----------------------------------------------------------------
 |  Copyright (c) 2012-, Tommy Sprinkle (tommy@tommysprinkle.com)
 |  Licensed under the NEW BSD Open Source License
  -----------------------------------------------------------------*/

typedef struct REGS
{
    unsigned int R0;
    unsigned int R1;
    unsigned int R15;
} REGS;

void EXSVC(int svc, REGS *regs);

void EXEC(REGS *regs);

#endif
