/*-----------------------------------------------------------------------------
|  Copyright (c) 2012-2013, Tommy Sprinkle (tommy@tommysprinkle.com)
|
|  All rights reserved. Redistribution and use in source and binary forms,
|  with or without modification, are permitted provided that the following
|  conditions are met:
|
|    * Redistributions of source code must retain the above copyright notice,
|      this list of conditions and the following disclaimer.
|    * Redistributions in binary form must reproduce the above copyright
|      notice, this list of conditions and the following disclaimer in the
|      documentation and/or other materials provided with the distribution.
|    * Neither the name of the author nor the names of its contributors may
|      be used toendorse or promote products derived from this software
|      without specific prior written permission.
|
|   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
|   "AS IS" AND ANYEXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
|   LIMITED TO, THE IMPLIED WARRANTIESOF MERCHANTABILITY AND FITNESS
|   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENTSHALL THE
|   COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
|   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
|   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
|   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
|   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
|   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
|   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
|   OF THE POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------*/

//---------------------------------------------------------------
// Dynamic Allocation (SVC 99) Interface
//
// Tommy Sprinkle - tommy@tommysprinkle.com
// December, 2012
//
//---------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "svc.h"

void DEAD(unsigned int);

//---------------------------------------------------------------
// Allocate A Dataset
//---------------------------------------------------------------
char *dynAlloc(char *dsn, char *rtddn, char *volser, char *device)
{
   static char errMsg[80];

   struct sS99RBPTR
   {
      struct sS99RB *rbptr;
   } S99RBPTR;

   struct sS99RB
   {
      unsigned char len;
      unsigned char type;
      unsigned short flags1;
      unsigned short error;
      unsigned short info;
      struct sTUPL *tupl;
      unsigned int rsv;
      unsigned int flags2;
   } S99RB;

   struct sTUPL
   {
      struct sTUDSN *tudsn;
      struct sTUSTAT *tustat;
      struct sTUDISP *tudisp;
      struct sTURTDDN *turtddn;
      struct sTUVOL *tuvol;
      struct sTUDEV *tudev;
   } TUPL;

   struct sTUDSN
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      char dsn[44];
   } TUDSN;

   struct sTUSTAT
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      unsigned char stat;
   } TUSTAT;

   struct sTUDISP
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      unsigned char disp;
   } TUDISP;

   struct sTURTDDN
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      char ddn[8];
   } TURTDDN;

   struct sTUVOL
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      char volser[6];
   } TUVOL;

   struct sTUDEV
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      char device[8];
   } TUDEV;

   struct REGS regs;
   int dsnlen;
   int volserlen;
   int devlen;
   unsigned char *p;

   if (dsn)
   {
      dsnlen = strlen(dsn);
      if (dsnlen > 44)
      {
         strcpy(errMsg, "DSNAME too long");
         return errMsg;
      }
   }

   if (volser)
   {
      volserlen = strlen(volser);
      if (volserlen > 6)
      {
         strcpy(errMsg, "VOLSER too long");
         return errMsg;
      }
   }

   if (device)
   {
      devlen = strlen(device);
      if (devlen > 8)
      {
         strcpy(errMsg, "DEVICE Type too long");
         return errMsg;
      }
   }

   memset(TUDSN.dsn, ' ', 44);

   if (dsn)
      memcpy(TUDSN.dsn, dsn, dsnlen);

   S99RBPTR.rbptr = &S99RB;
   p = (unsigned char *)&S99RBPTR.rbptr;
   *p = 0x80;

   S99RB.len = 20;
   S99RB.type = 1;
   S99RB.flags1 = 0x6000;
   S99RB.error = 0;
   S99RB.info = 0;
   S99RB.tupl = &TUPL;
   S99RB.rsv = 0;
   S99RB.flags2 = 0;

   if (dsn)
      TUPL.tudsn = &TUDSN;
   else
      TUPL.tudsn = NULL;
   TUPL.tustat = &TUSTAT;
   TUPL.tudisp = &TUDISP;
   TUPL.turtddn = &TURTDDN;
   TUPL.tuvol = &TUVOL;
   TUPL.tudev = &TUDEV;

   p = (unsigned char *)&TUPL.tudev;
   *p = 0x80;

   TUDSN.code = 0x0002;
   TUDSN.count = 1;
   TUDSN.length = 44;

   TUSTAT.code = 0x0004;
   TUSTAT.count = 1;
   TUSTAT.length = 1;
   TUSTAT.stat = 8; // SHR

   TUDISP.code = 0x0005;
   TUDISP.count = 1;
   TUDISP.length = 1;
   TUDISP.disp = 8; // KEEP

   TURTDDN.code = 0x0055;
   TURTDDN.count = 1;
   TURTDDN.length = 8;
   memcpy(TURTDDN.ddn, "        ", 8);

   // If volser and device are specified...
   if (volser && device && strlen(volser) && strlen(device))
   {
      TUVOL.code = 0x0010;
      TUVOL.count = 1;
      TUVOL.length = 6;
      memset(TUVOL.volser, ' ', 6);
      memcpy(TUVOL.volser, volser, volserlen);

      TUDEV.code = 0x0015;
      TUDEV.count = 1;
      TUDEV.length = 8;
      memset(TUDEV.device, ' ', 8);
      memcpy(TUDEV.device, device, devlen);
   }
   else
   {
      p = (unsigned char *)&TUPL.turtddn;
      *p = 0x80;
   }

   regs.R1 = (unsigned int)&S99RBPTR;

   EXSVC(99, &regs);

   if (regs.R15)
   {
      sprintf(errMsg, "RC=%d ERR=%4.4X INFO=%4.4X", regs.R15, S99RB.error, S99RB.info);
#if 0
      fprintf(stderr, "Dynamic Allocation Request Failed\n");
      fprintf(stderr, "Allocate DSN=%44.44s\n",TUDSN.dsn);
      fprintf(stderr, "Return code from SVC 99 is %d\n",regs.R15);
      fprintf(stderr, "S99ERROR = %4.4X\n",S99RB.error);
      fprintf(stderr, "S99INFO  = %4.4X\n",S99RB.info);
#endif
      return errMsg;
   }
   else
   {
      memcpy(rtddn, TURTDDN.ddn, 8);
      *(rtddn + 8) = '\0';
   }

   return NULL;
}

//---------------------------------------------------------------
// Free A Dataset
//---------------------------------------------------------------
char *dynFree(char *ddn)
{
   static char errMsg[80];

   struct sS99RBPTR
   {
      struct sS99RB *rbptr;
   } S99RBPTR;

   struct sS99RB
   {
      unsigned char len;
      unsigned char type;
      unsigned short flags1;
      unsigned short error;
      unsigned short info;
      struct sTUPL *tupl;
      unsigned int rsv;
      unsigned int flags2;
   } S99RB;

   struct sTUPL
   {
      struct sTUDDN *tuddn;
   } TUPL;

   struct sTUDDN
   {
      unsigned short code;
      unsigned short count;
      unsigned short length;
      char ddn[8];
   } TUDDN;

   struct REGS regs;
   int ddnlen;
   unsigned char *p;

   ddnlen = strlen(ddn);
   if (ddnlen > 8)
   {
      strcpy(errMsg, "DDNAME too long");
      return errMsg;
   }

   memset(TUDDN.ddn, ' ', 44);

   memcpy(TUDDN.ddn, ddn, ddnlen);

   S99RBPTR.rbptr = &S99RB;
   p = (unsigned char *)&S99RBPTR.rbptr;
   *p = 0x80;

   S99RB.len = 20;
   S99RB.type = 2;
   S99RB.flags1 = 0x6000;
   S99RB.error = 0;
   S99RB.info = 0;
   S99RB.tupl = &TUPL;
   S99RB.rsv = 0;
   S99RB.flags2 = 0;

   TUPL.tuddn = &TUDDN;

   p = (unsigned char *)&TUPL.tuddn;
   *p = 0x80;

   TUDDN.code = 0x0001;
   TUDDN.count = 1;
   TUDDN.length = 8;

   regs.R1 = (unsigned int)&S99RBPTR;

   EXSVC(99, &regs);

   if (regs.R15)
   {
      sprintf(errMsg, "RC=%d ERR=%4.4X INFO=%4.4X", regs.R15, S99RB.error, S99RB.info);
#if 0
      fprintf(stderr, "Dynamic Allocation Request Failed\n");
      fprintf(stderr, "Free DDNAME=%8.8s\n",TUDDN.ddn);
      fprintf(stderr, "Return code from SVC 99 is %d\n",regs.R15);
      fprintf(stderr, "S99ERROR = %4.4X\n",S99RB.error);
      fprintf(stderr, "S99INFO  = %4.4X\n",S99RB.info);
#endif
      return errMsg;
   }

   return NULL;
}
