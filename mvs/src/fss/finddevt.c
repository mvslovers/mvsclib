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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *checkVOL(unsigned char *UCB, char *volser);

char *finddevt(char *volser)
{
	unsigned char **CVTPTR;
	unsigned char *CVT;
	unsigned char *UCBHW;
	unsigned char *UCB;
	char *devt;

	CVTPTR = (unsigned char **)0x10;

	CVT = *CVTPTR;

	UCBHW = *((unsigned char **)(CVT + 0x28));

	while (1)
	{
		UCB = (unsigned char *)(*UCBHW << 8) + *(UCBHW + 1);
		if ((unsigned int)UCB == 0xFFFF)
			break;

		if (UCB != NULL)
		{
			devt = checkVOL(UCB, volser);
			if (devt != NULL)
				return devt;
		}
		UCBHW = UCBHW + 2;
	}

	return NULL;
}

static char *checkVOL(unsigned char *UCB, char *volser)
{
	struct sUCBTYPE
	{
		unsigned char code;
		char *type;
	};

	static struct sUCBTYPE DATYPE[] =
		{
			{0x01, "3211"},
			{0x02, "2301"},
			{0x03, "2303"},
			{0x04, "2302"},
			{0x05, "2321"},
			{0x06, "2305"},
			{0x07, "2305"},
			{0x08, "2314"},
			{0x09, "3330"},
			{0x0A, "3340"},
			{0x0B, "3350"},
			{0x0D, "3330"},
			{0xFF, ""}};

	int i;

	if (*(UCB + 0x12) != 0x20)
		return NULL; // Not DASD

	if (memcmp(volser, UCB + 0x1C, 6) != 0)
		return NULL;

	for (i = 0;; i++)
	{
		if (DATYPE[i].code == 0xFF)
		{
			return NULL;
		}
		if (DATYPE[i].code == *(UCB + 0x13))
		{
			return DATYPE[i].type;
		}
	}

	// return NULL;
}
