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
// EXCP I/O Read-only Interface - Block Level Access
//
// Tommy Sprinkle - tommy@tommysprinkle.com
// December, 2012
//
//---------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "svc.h"
#include "excp.h"

#define DCBLEN 52
#define MAXLEN 47968

void DEAD(unsigned int);

struct sIOB
{
   unsigned char flags[2];
   unsigned char sense[2];
   unsigned int *ecb;
   unsigned char *csw;
   unsigned char cswflag[2];
   unsigned short residual;
   unsigned char *ccw;
   unsigned char *dcb;
   unsigned char rsv[8];
   unsigned char mbbcchhr[8];
};

struct sEXCPFILE
{
   char eyeball[8];
   unsigned char *dcb;
   unsigned int exlst;
   unsigned char *jfcb;
   unsigned char *ccw;
   unsigned char *buf;
   unsigned int ecb;
   struct sIOB *iob;

   unsigned char mbbcchhr[8];
};

// Private Functions
static int makeDCB(unsigned char *dcb, char *ddname, unsigned int *exlst);
static int readJFCB(unsigned char *dcb);
static int openDCB(unsigned char *dcb);
static int openJDCB(unsigned char *dcb);
static int closeDCB(unsigned char *dcb);
static int cnvtTTR(unsigned char *dcb, int ttr, unsigned char *MBBCCHHR);
static int buildIOB(struct sIOB *iob, unsigned char *dcb,
                    unsigned int *ecb, unsigned char *ccw,
                    unsigned char *mbbcchhr);
static int makeCCW(unsigned char *ccw, struct sIOB *iob, unsigned char *buf);
static int makeCCWTRK(unsigned char *ccw, struct sIOB *iob, unsigned char *buf);
static int doEXCP(struct sIOB *iob, unsigned char *buf, int *kl, int *dl);
static int doEXCPTRK(struct sIOB *iob, unsigned char *buf, int *kl, int *dl);

// Public Functions
excpfile xOPEN(char *ddname);
excpfile xOPENV(char *ddname);
int xCLOSE(excpfile ef);
int xREAD(excpfile ef, struct sEXCP *excp, int tt, int r);
int xTREAD(excpfile ef, struct sEXCP *excp, int tt);

excpfile xOPEN(char *ddname)
{
   struct sEXCPFILE *excpf;
   int rc;

   excpf = (struct sEXCPFILE *)malloc(sizeof(struct sEXCPFILE));
   memcpy(excpf->eyeball, "EXCPFILE", 8); // Set EyeBall

   excpf->jfcb = (unsigned char *)malloc(176);              // JFCB
   excpf->dcb = (unsigned char *)malloc(DCBLEN);            // DCB
   excpf->iob = (struct sIOB *)malloc(sizeof(struct sIOB)); // IOB
   excpf->ccw = (unsigned char *)malloc(24);                // 3 CCW's
   excpf->buf = (unsigned char *)malloc(MAXLEN);            // I/O Buffer

   excpf->exlst = (unsigned int)excpf->jfcb; // Build DCB EXLST
   excpf->exlst |= 0x87000000;               // X'87",AL3(JFCB)

   rc = makeDCB(excpf->dcb, ddname, &(excpf->exlst)); // Make a DCB

   if (!rc)
      rc = openDCB(excpf->dcb); // Issue OPEN SVC

   if (rc) // If open failed... cleanup
   {
      free(excpf->jfcb);
      free(excpf->dcb);
      free(excpf->iob);
      free(excpf->ccw);
      free(excpf->buf);
      free(excpf);

      return 0;
   }

   return excpf; // Return ExcpFile *
}

excpfile xOPENV(char *ddname)
{
   struct sEXCPFILE *excpf;
   int rc;

   excpf = (struct sEXCPFILE *)malloc(sizeof(struct sEXCPFILE));
   memcpy(excpf->eyeball, "EXCPFILE", 8); // Set EyeBall

   excpf->jfcb = (unsigned char *)malloc(176);              // JFCB
   excpf->dcb = (unsigned char *)malloc(DCBLEN);            // DCB
   excpf->iob = (struct sIOB *)malloc(sizeof(struct sIOB)); // IOB
   excpf->ccw = (unsigned char *)malloc(24);                // 3 CCW's
   excpf->buf = (unsigned char *)malloc(MAXLEN);            // I/O Buffer

   excpf->exlst = (unsigned int)excpf->jfcb; // Build DCB EXLST
   excpf->exlst |= 0x87000000;               // X'87",AL3(JFCB)

   rc = makeDCB(excpf->dcb, ddname, &(excpf->exlst)); // Make a DCB

   readJFCB(excpf->dcb);

   memset(excpf->jfcb, 0x04, 44);

   if (!rc)
      rc = openJDCB(excpf->dcb); // Issue OPEN SVC

   if (rc) // If open failed... cleanup
   {
      free(excpf->jfcb);
      free(excpf->dcb);
      free(excpf->iob);
      free(excpf->ccw);
      free(excpf->buf);
      free(excpf);

      return 0;
   }

   return excpf; // Return ExcpFile *
}

int xCLOSE(excpfile ef)
{
   struct sEXCPFILE *excpf;
   int rc;

   excpf = ef;

   if (memcmp(excpf->eyeball, "EXCPFILE", 8)) // Check for EyeBall
      return -100;

   rc = closeDCB(excpf->dcb); // Issue Close SVC

   free(excpf->jfcb); // Free Storage
   free(excpf->dcb);
   free(excpf->iob);
   free(excpf->ccw);
   free(excpf->buf);
   free(excpf);

   return rc;
}

int xREAD(excpfile ef, struct sEXCP *excp, int tt, int r)
{
   struct sEXCPFILE *excpf;
   int rc;

   excpf = (struct sEXCPFILE *)ef;
   if (memcmp(excpf->eyeball, "EXCPFILE", 8)) // Validate EyeBall
      return -100;

   if (r < 1) // Record 0 is invalid
      return -101;

   rc = cnvtTTR(excpf->dcb, (tt << 8) + (r - 1), excpf->mbbcchhr); // Convert TTR
   if (rc)
      return rc;

   // Build IOB and CCWs then Issue EXCP
   buildIOB(excpf->iob, excpf->dcb, &excpf->ecb, excpf->ccw, excpf->mbbcchhr);
   makeCCW(excpf->ccw, excpf->iob, excpf->buf);
   rc = doEXCP(excpf->iob, excpf->buf, &excp->keylen, &excp->datalen);

   if (!rc)
   {
      excp->count = excpf->buf;                       // Point to count
      excp->key = (excpf->buf) + 8;                   // Point to Key
      excp->data = (excpf->buf) + 8 + (excp->keylen); // Point to Data
      memcpy(excp->mbbcchhr, excpf->mbbcchhr, 3);     // Copy MBB
      memcpy((excp->mbbcchhr) + 3, excpf->buf, 5);    // Copy CCHHR from Count
   }

   return rc;
}

int xTREAD(excpfile ef, struct sEXCP *excp, int tt)
{
   struct sEXCPFILE *excpf;
   int rc;

   excpf = (struct sEXCPFILE *)ef;
   if (memcmp(excpf->eyeball, "EXCPFILE", 8)) // Validate EyeBall
      return -100;

   rc = cnvtTTR(excpf->dcb, (tt << 8), excpf->mbbcchhr); // Convert TTR (R=0)
   if (rc)
      return rc;

   // Build IOB and CCWs then Issue EXCP
   buildIOB(excpf->iob, excpf->dcb, &excpf->ecb, excpf->ccw, excpf->mbbcchhr);
   makeCCWTRK(excpf->ccw, excpf->iob, excpf->buf);
   rc = doEXCPTRK(excpf->iob, excpf->buf, &excp->keylen, &excp->datalen);

   if (!rc)
   {
      excp->count = excpf->buf;                    // Point to count
      excp->key = excpf->buf;                      // Point to Key
      excp->data = excpf->buf;                     // Point to Data
      memcpy(excp->mbbcchhr, excpf->mbbcchhr, 3);  // Copy MBB
      memcpy((excp->mbbcchhr) + 3, excpf->buf, 5); // Copy CCHHR from Count
   }

   return rc;
}

static int makeDCB(unsigned char *dcb, char *ddname, unsigned int *exlst)
{
   int len;
   unsigned int **wkptr;

   // DCB for EXCP is 52 bytes
   memset(dcb, '\0', 52); // zero dcb storage

   *(dcb + 23) = 0x01; // BUFCB
   *(dcb + 26) = 0x40;
   *(dcb + 31) = 0x01; // IOBAD
   *(dcb + 35) = 0x01; // EODAD
   *(dcb + 48) = 0x02; // OFLGS
   *(dcb + 50) = 0xD0;
   *(dcb + 51) = 0x08;

   wkptr = (unsigned int **)(dcb + 36);
   *wkptr = exlst;

   memset(dcb + 40, ' ', 8);
   len = strlen(ddname);
   if (len > 8)
      return -1;

   memcpy(dcb + 40, ddname, len);

   return 0;
}

static int openDCB(unsigned char *dcb)
{
   unsigned int parmlist;
   struct REGS regs;

   parmlist = (unsigned int)dcb; // Build parmlist for SVC 19
   parmlist |= 0x80000000;       // X'80',AL3(DCB)

   regs.R1 = (unsigned int)&parmlist; // LA R1,PARMLIST

   EXSVC(19, &regs); // Issue Open SVC

   if ((*(dcb + 48) & 0x10) == 0x10) // Did DCB Open?
      return 0;

   return 8;
}

static int openJDCB(unsigned char *dcb)
{
   unsigned int parmlist;
   struct REGS regs;

   parmlist = (unsigned int)dcb; // Build parmlist for SVC 19
   parmlist |= 0x80000000;       // X'80',AL3(DCB)

   regs.R1 = (unsigned int)&parmlist; // LA R1,PARMLIST

   EXSVC(22, &regs); // Issue Open TYPE=J SVC

   if ((*(dcb + 48) & 0x10) == 0x10) // Did DCB Open?
      return 0;

   return 8;
}

static int readJFCB(unsigned char *dcb)
{
   unsigned int parmlist;
   struct REGS regs;

   parmlist = (unsigned int)dcb; // Build parmlist for SVC 19
   parmlist |= 0x80000000;       // X'80',AL3(DCB)

   regs.R1 = (unsigned int)&parmlist; // LA R1,PARMLIST

   EXSVC(64, &regs); // Issue RDJFCB SVC

   return 8;
}

static int closeDCB(unsigned char *dcb)
{
   unsigned int parmlist;
   struct REGS regs;

   parmlist = (unsigned int)dcb; // Build parmlist for SVC 20
   parmlist |= 0x80000000;       // X'80',AL3(DCB)

   regs.R1 = (unsigned int)&parmlist; // LA R1,PARMLIST

   EXSVC(20, &regs); // Issue Close SVC

   return 0;
}

static int cnvtTTR(unsigned char *dcb, int ttr, unsigned char *mbbcchhr)
{

   struct sXtent // Extent Entry In DEB
   {
      void *ucb;
      unsigned short bin;
      unsigned short startCC;
      unsigned short startHH;
      unsigned short endCC;
      unsigned short endHH;
      unsigned short tracks;
   };

   unsigned char *deb;
   unsigned char **pdeb;
   struct sXtent *xtent;

   int m;
   int cc;
   int hh;
   int r;
   int tt;
   int extents;
   int tracksPerCyl;

   tracksPerCyl = 30; // Hard Coded For 3350

   r = ttr & 0xFF;           // Isolate Record
   tt = (ttr >> 8) & 0xFFFF; // Isolate Track

   pdeb = (unsigned char **)(dcb + 44); // Point to DEB
   deb = *pdeb;
   extents = (unsigned char)*(deb + 16); // Get number of Extents
   xtent = (struct sXtent *)(deb + 32);  // Point to first Extent

   m = 0;

   while (extents && tt >= xtent->tracks) // Find Extent for Request
   {
      tt -= xtent->tracks; // Subtract tracks in extent
      xtent++;             // Next Extent Entry
      m++;                 // Increment M
      extents--;           // Decrement Extents
   }

   if (!extents) // Did we find an Extent?
   {
      return 12; // TTR Outside Extent
   }

   cc = xtent->startCC; // Start CC for extent
   hh = xtent->startHH; // Start HH for extent

   hh += tt;                // Offset into extent
   cc += hh / tracksPerCyl; // Clean up CCHH
   hh = hh % tracksPerCyl;

   memset(mbbcchhr, '\0', 8); // Build MBBCCHHR

   mbbcchhr[0] = m;
   mbbcchhr[3] = cc >> 8;
   mbbcchhr[4] = cc & 0xFF;
   mbbcchhr[5] = hh >> 8;
   mbbcchhr[6] = hh & 0xFF;
   mbbcchhr[7] = r;

   return 0;
}

static int buildIOB(struct sIOB *iob, unsigned char *dcb,
                    unsigned int *ecb, unsigned char *ccw,
                    unsigned char *mbbcchhr)
{
   // Zero out IOB storage
   memset(iob, '\0', sizeof(struct sIOB));

   iob->dcb = dcb;       // set DCB address
   iob->ecb = ecb;       // set ECB address
   iob->ccw = ccw;       // point to CCW program
   iob->flags[0] = 0x43; // set flags
   memcpy(iob->mbbcchhr, mbbcchhr, 8);

   return 0;
}

static int makeCCW(unsigned char *ccw, struct sIOB *iob, unsigned char *buf)
{
   unsigned char *p;

   p = &(iob->mbbcchhr[3]); // CCHHR
   *((void **)ccw) = p;
   *ccw = 0x31;       // Search ID Equal
   *(ccw + 4) = 0x40; // Command Chain
   *(ccw + 5) = 0x00;
   *(ccw + 6) = 0x00;
   *(ccw + 7) = 0x05;

   p = ccw;
   ccw += 8;            // Next CCW
   *((void **)ccw) = p; // Address of Search CCW
   *ccw = 0x08;         // TIC
   *(ccw + 4) = 0x40;   // Command Chain
   *(ccw + 5) = 0x00;
   *(ccw + 6) = 0x00;
   *(ccw + 7) = 0x01;

   ccw += 8;              // Next CCW
   *((void **)ccw) = buf; // I/O Buffer Address
   *ccw = 0x1E;           // Read Count, Key, Data
   *(ccw + 4) = 0x20;     // Suppress Incorrect Length
   *(ccw + 5) = 0x00;
   *(ccw + 6) = (MAXLEN >> 8) & 0xFF; // Set Max Record Length
   *(ccw + 7) = MAXLEN & 0xFF;

   return 0;
}

static int makeCCWTRK(unsigned char *ccw, struct sIOB *iob, unsigned char *buf)
{
   unsigned char *p;

   p = &(iob->mbbcchhr[3]); // CCHHR
   *((void **)ccw) = p;
   *ccw = 0x31;       // Search ID Equal
   *(ccw + 4) = 0x40; // Command Chain
   *(ccw + 5) = 0x00;
   *(ccw + 6) = 0x00;
   *(ccw + 7) = 0x05;

   p = ccw;
   ccw += 8;            // Next CCW
   *((void **)ccw) = p; // Address of Search CCW
   *ccw = 0x08;         // TIC
   *(ccw + 4) = 0x40;   // Command Chain
   *(ccw + 5) = 0x00;
   *(ccw + 6) = 0x00;
   *(ccw + 7) = 0x01;

   ccw += 8;              // Next CCW
   *((void **)ccw) = buf; // I/O Buffer Address
   *ccw = 0x5E;           // Read Track
   *(ccw + 4) = 0x20;     // Suppress Incorrect Length
   *(ccw + 5) = 0x00;
   *(ccw + 6) = (MAXLEN >> 8) & 0xFF; // Set Max Record Length
   *(ccw + 7) = MAXLEN & 0xFF;

   return 0;
}

static int doEXCP(struct sIOB *iob, unsigned char *buf, int *kl, int *dl)
{
   unsigned int *ecb;
   struct REGS regs;
   int cc;
   int r;
   int reclen;
   int keylen;

   ecb = iob->ecb;
   *ecb = 0; // Clear ECB

   regs.R1 = (unsigned int)iob; // LA R1,IOB
   EXSVC(0, &regs);             // Issue EXCP

   regs.R1 = (unsigned int)ecb; // LA R1,ECB
   regs.R0 = 1;                 // LA R0,1
   EXSVC(1, &regs);             // Wait for I/O to complete

   cc = *ecb >> 24; // Get completion code from ECB

   if (cc == 0x42) // Was record an EOF
   {
      return 8; // Indicate EOF
   }

   if (cc == 0x41) // Data Check ?
   {
      if ((iob->cswflag[0] & 0x02) == 0x02) // End of Track ?
         return 4;
      if ((iob->cswflag[0] & 0x01) == 0x01) // End of File ?
         return 8;
      return 16; // Error
   }

   if (cc != 0x7F) // 7F - no error
   {
      return 16; // Error
   }

   r = iob->mbbcchhr[7]; // Record number looking for
   r++;                  // add one
   if (r != *(buf + 4))  // Record we were expecting?
      return 4;

   *kl = *(buf + 5);                     // get key length from count area
   *dl = (*(buf + 6) << 8) + *(buf + 7); // get data length from count area

   return 0;
}

static int doEXCPTRK(struct sIOB *iob, unsigned char *buf, int *kl, int *dl)
{
   unsigned int *ecb;
   struct REGS regs;
   int cc;
   int r;
   int reclen;
   int keylen;

   ecb = iob->ecb;
   *ecb = 0; // Clear ECB

   regs.R1 = (unsigned int)iob; // LA R1,IOB
   EXSVC(0, &regs);             // Issue EXCP

   regs.R1 = (unsigned int)ecb; // LA R1,ECB
   regs.R0 = 1;                 // LA R0,1
   EXSVC(1, &regs);             // Wait for I/O to complete

   cc = *ecb >> 24; // Get completion code from ECB

   if (cc == 0x42) // Was record an EOF
   {
      return 8; // Indicate EOF
   }

   if (cc == 0x41) // Data Check ?
   {
      if ((iob->cswflag[0] & 0x02) == 0x02) // End of Track ?
         return 4;
      if ((iob->cswflag[0] & 0x01) == 0x01) // End of File ?
         return 8;
      return 16; // Error
   }

   if (cc != 0x7F) // 7F - no error
   {
      return 16; // Error
   }

   *kl = 0;                      // No KEY
   *dl = MAXLEN - iob->residual; // Set DL to Length of data read

   return 0;
}
