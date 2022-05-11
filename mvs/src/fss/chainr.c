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
// Linked List Chainer
//
// Tommy Sprinkle - tommy@tommysprinkle.com
// January, 2013
//
//---------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chainr.h"

//-----------------------------------------------------------------------
// Create new Chain
//-----------------------------------------------------------------------
CHAINR chainNewFunc(int keyLen, int keyType, int recSize, int allocSize)
{
	CHAINR scr;

	scr = (CHAINR)malloc(sizeof(struct sChainR)); // New CHAINR Struct

	if (keyType != 0 && keyType != 1 && keyType != 2) // Validate Key Type
	{
		fprintf(stderr, "CHAINR - Invalid Key Type\n");
		return NULL;
	}

	if (keyLen < 1 || keyLen > 255) // Validate Key Length
	{
		fprintf(stderr, "CHAINR - Invalid Key Length\n");
		return NULL;
	}

	if (allocSize < 512) // Minimum Block Allocation Size
		allocSize = 512;

	if (recSize > (allocSize / 5)) // Allocation Block Minimum Size
	{
		fprintf(stderr, "CHAINR - Allocation Size Too Small\n");
		return NULL;
	}

	memcpy(scr->eyeball, "CHNR", 4); // Set CB Identifier
	scr->keyType = keyType;			 // Set Key Type
	scr->keyLen = keyLen;			 // Set Key Length
	scr->recSize = recSize;			 // Set Record Size
	scr->allocSize = allocSize;		 // Set Block Allocation Size
	scr->entryCount = 0;			 // Count of Entries is Zero

	scr->first = NULL; // Chain Is
	scr->last = NULL;  //         Empty

	scr->firstBlk = (struct sChainRblk *)malloc(allocSize); // Allocate a Block
	scr->activeBlk = scr->firstBlk;							// It is the Active Block
	scr->freeBlk = NULL;									// No Free Blocks

	scr->firstBlk->next = NULL;												   // Last Block
	scr->firstBlk->data = ((char *)scr->firstBlk) + sizeof(struct sChainRblk); // Next Data Ptr
	scr->firstBlk->free = allocSize - sizeof(struct sChainRblk);			   // Free Length

	return scr; // Return to Caller
}

//-----------------------------------------------------------------------
// Reset a Chain - Don't Free Storage
//-----------------------------------------------------------------------
int chainReset(CHAINR scr)
{
	struct sChainRblk *block;
	struct sChainRblk *nxtBlk;

	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainFree()\n");
		abort();
	}

	block = scr->firstBlk;									   // Point to in-use block chain
	scr->activeBlk = block;									   // Make it Active Block
	block->data = ((char *)block) + sizeof(struct sChainRblk); // reset Data ptr
	block->free = scr->allocSize - sizeof(struct sChainRblk);  // reset Size

	nxtBlk = block->next; // save addr of next block
	block->next = 0;	  // clear Fwd ptr
	block = nxtBlk;		  // First block to move to free list

	while (block != NULL) // Loop through blocks
	{
		nxtBlk = block->next; // Save next block ptr

		block->data = ((char *)block) + sizeof(struct sChainRblk); // reset Data ptr
		block->free = scr->allocSize - sizeof(struct sChainRblk);  // reset Size

		block->next = scr->freeBlk; // Add to
		scr->freeBlk = block;		//      Free Chain

		block = nxtBlk; // Process Next Block
	}

	scr->first = NULL;
	scr->last = NULL;
	scr->entryCount = 0;

	return 0;
}

//-----------------------------------------------------------------------
// Free a Chain
//-----------------------------------------------------------------------
int chainFree(CHAINR scr)
{
	struct sChainRblk *block;
	struct sChainRblk *nxtBlk;

	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainFree()\n");
		abort();
	}

	block = scr->firstBlk; // Point to in-use block chain

	while (block != NULL) // Loop through chain
	{
		nxtBlk = block->next; // Save next block address
		block->next = NULL;	  // Clear Fwd Pointer
		block->data = NULL;	  // Clear Data Ptr
		block->free = 0;	  // Clear Free Size
		free(block);		  // Free Block Storage
		block = nxtBlk;		  // And on to next
	}

	block = scr->freeBlk; // Point to free block chain

	while (block != NULL) // Loop thorugh chain
	{
		nxtBlk = block->next; // Save next block address
		block->next = NULL;	  // Clear Fwd Pointer
		block->data = NULL;	  // Clear Data ptr
		block->free = 0;	  // Clear Free size
		free(block);		  // Free Block Storage
		block = nxtBlk;		  // Process next
	}

	scr->first = NULL;	   // Clear
	scr->last = NULL;	   //     Chain
	scr->firstBlk = NULL;  //          Pointers
	scr->activeBlk = NULL; //
	free(scr);			   // Freee Storage

	return 0;
}

//-----------------------------------------------
// Return Ptr to First Element
//-----------------------------------------------
char *chainFirst(CHAINR scr)
{
	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainFirst()\n");
		abort();
	}

	return (char *)scr->first;
}

//-----------------------------------------------
// Return Ptr to Last Element
//-----------------------------------------------
char *chainLast(CHAINR scr)
{
	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainFirst()\n");
		abort();
	}

	return (char *)scr->last;
}

//-----------------------------------------------
// Return Count of Elements
//-----------------------------------------------
int chainCount(CHAINR scr)
{
	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainCount()\n");
		abort();
	}

	return scr->entryCount;
}

//-----------------------------------------------
// Find An Element By Key
//-----------------------------------------------
char *chainFindFunc(CHAINR scr, void *key)
{
	int i;
	char *cval;
	struct sChainRhdr *pHdr;
	char wKey[256];

	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainFind()\n");
		abort();
	}

	if (scr->keyType == 0) // INT
	{
		cval = (char *)&key; // Get Ptr to INT
	}
	else if (scr->keyType == 1) // CHAR
	{
		i = strlen((char *)key);		  // String Length
		if (i >= scr->keyLen)			  // Equal to Or Greter than Key Len
			memcpy(wKey, (char *)key, i); // Copy In
		else
		{
			memset(wKey, ' ', scr->keyLen); // Pad with Blanks
			memcpy(wKey, (char *)key, i);	// Copy Key Data
		}
		cval = wKey; // Address of Key
	}
	else // Binary
	{
		cval = (char *)key; // Address of Key
	}

	pHdr = scr->first;
	while (pHdr != NULL)
	{
		i = memcmp(cval, &(pHdr->key), scr->keyLen); // Check for Key Match
		if (i == 0)
		{
			return (char *)pHdr; // Return ptr to Element
		}

		pHdr = pHdr->next; // Next Element on Chain
	}

	return NULL; // Key not Found - Return NULL
}

//-----------------------------------------------
// Add A New Element
//-----------------------------------------------
char *chainAddFunc(CHAINR scr, void *key)
{
	struct sChainRblk *newBlk;
	char *newRec;
	struct sChainRhdr *newHdr;
	struct sChainRhdr *pHdr;
	int ival;
	int i;
	int cmpKeyToHead;
	int cmpKeyToTail;
	char *cval;

	char wKey[256];

	if (memcmp(scr->eyeball, "CHNR", 4))
	{
		fprintf(stderr, "Invalid Chain Control Block - chainAdd()\n");
		abort();
	}

	if (scr->keyType == 0) // INT
	{
		cval = (char *)&key; // Get Ptr to INT
	}
	else if (scr->keyType == 1) // CHAR
	{
		i = strlen((char *)key);		  // String Length
		if (i >= scr->keyLen)			  // Equal to Or Greter than Key Len
			memcpy(wKey, (char *)key, i); // Copy In
		else
		{
			memset(wKey, ' ', scr->keyLen); // Pad with Blanks
			memcpy(wKey, (char *)key, i);	// Copy Key Data
		}
		cval = wKey; // Address of Key
	}
	else // Binary
	{
		cval = (char *)key; // Address of Key
	}

	if (scr->recSize > scr->activeBlk->free) // Allocate New Block If Needed
	{
		if (scr->freeBlk != NULL)
		{
			newBlk = scr->freeBlk;
			scr->freeBlk = newBlk->next;
		}
		else
		{
			newBlk = (struct sChainRblk *)malloc(scr->allocSize);
		}

		newBlk->next = NULL;										 // Last Block
		newBlk->data = ((char *)newBlk) + sizeof(struct sChainRblk); // Ptr to Data
		newBlk->free = scr->allocSize - sizeof(struct sChainRblk);	 // Free Length
		scr->activeBlk->next = newBlk;								 // Chain on to Previous Block
		scr->activeBlk = newBlk;									 // Now the Active Block
	}

	newRec = scr->activeBlk->data;		  // Point to New Element
	scr->activeBlk->data += scr->recSize; // Adjust Free Data Ptr
	scr->activeBlk->free -= scr->recSize; // Adjust Free Length

	newHdr = (struct sChainRhdr *)newRec; // Map It

	cmpKeyToTail = memcmp(cval, &(scr->last->key), scr->keyLen);
	cmpKeyToHead = memcmp(cval, &(scr->first->key), scr->keyLen);

	if (scr->first == NULL) // Special Case - Chain Is Empty
	{
		scr->first = newHdr;
		scr->last = newHdr;
		newHdr->next = NULL;
		newHdr->prev = NULL;
	}
	else if (cmpKeyToTail > 0) // Special Case - Add to Tail
	{
		newHdr->prev = scr->last;
		scr->last->next = newHdr;
		scr->last = newHdr;
		newHdr->next = NULL;
	}
	else if (cmpKeyToHead < 0) // Special Case - Add to Head
	{
		newHdr->next = scr->first;
		scr->first->prev = newHdr;
		scr->first = newHdr;
		newHdr->prev = NULL;
	}
	else // Add To Middle of Chain
	{
		pHdr = scr->first;
		while (1)
		{
			i = memcmp(cval, &(pHdr->key), scr->keyLen); // Check for Duplicate
			if (i == 0)
			{
				printf("+++DUPLICATE+++\n");
				return NULL; // DUPLICATE
			}

			if (i < 0) // Found Where to Add
				break;

			pHdr = pHdr->next; // Next Element on Chain
			if (pHdr == NULL)  // Should Never Happen
			{				   //  Add to Tail is a Special Case
				fprintf(stderr, "CHAINR - Logic Error 101\n");
				abort();
			}
		}
		newHdr->prev = pHdr->prev; // Add
		newHdr->next = pHdr;	   //    Element
		pHdr->prev->next = newHdr; //           To
		pHdr->prev = newHdr;	   //             Chain
	}

	scr->entryCount++;						   // Increment Element Count
	memcpy(&(newHdr->key), cval, scr->keyLen); // Copy In The Key

	return (char *)newHdr; // Return New Element
}

// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
// -------------------------------------------------------------------------------------------------
#if 0


void prtChain(CHAINR scr)
{
	struct sChainRhdr *rec;

	printf("\n\n---------- Chain ----------\n");
	printf("Head = %8.8X Tail = %8.8X\n",scr->first,scr->last);

	rec = (struct sChainRhdr *) scr->first;
	while(rec != NULL)
	{
		if(scr->keyType == 0)
		{
			printf("--%8.8X %8.8X %8.8X\n",rec->next, rec->prev, rec->key);
		}
		else
		{
			printf("--%8.8X %8.8X %*.*s\n",rec->next, rec->prev, 
				scr->keyLen, scr->keyLen, &(rec->key));
		}
		rec = rec->next;
	}
	printf("\n");

	return;
}


struct irec
{
	struct irec *next;
	struct irec *prev;
	char         key[8];
	int          count;
};


int main(int argc, char ** argv)
{
	CHAINR Chain;
	struct sChainR scr;

	//Chain = chainNewInt( 16 );
	Chain = chainNewChar( 8, sizeof(struct irec) );

	chAdd(Chain, "Tommy");
	prtChain(Chain);

	chAdd(Chain, "Donna");
	prtChain(Chain);

	chAdd(Chain, "Vince");
	prtChain(Chain);

	chAdd(Chain, "Jenny");
	prtChain(Chain);

	chAdd(Chain, "Talor");
	prtChain(Chain);

	chAdd(Chain, "Eli");
	prtChain(Chain);

	chAdd(Chain, "Si");
	prtChain(Chain);

	chAdd(Chain, "AAAAAAAA");
	chAdd(Chain, "ZZZZZZZZ");
	prtChain(Chain);

	chainFree(Chain);

	chAdd(&scr, "AAAAAAAA");


	//chAdd(CharChain, "aaaa");
	//chAdd(CharChain, "bbbb");
	//chAdd(CharChain, "cccc");



	return 0;
}

#endif
