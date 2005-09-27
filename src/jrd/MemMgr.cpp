/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#ifndef NO_MEM_INLINE
#define NO_MEM_INLINE
#endif

#include <stdio.h>
#if defined(MVS) || defined(__VMS)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "MemMgr.h"
#include "Sync.h"
#include "Mutex.h"
#include "OSRIMemException.h"
#include "Interlock.h"
#include "gen/iberror.h"


static Mutex				defaultMemMgrMutex;
static NAMESPACE::MemMgr	*defaultMemoryManager;


static const int guardBytes = sizeof(long); // * 2048;

#define INIT_BYTE			0xCC
#define GUARD_BYTE			0xDD
#define DELETE_BYTE			0xEE
#define ABS(n)				((n < 0) ? -n : n)

MemMgr* getDefaultMemoryManager()
	{
	if (!defaultMemoryManager)
		{
		defaultMemMgrMutex.lock();
		
		if (!defaultMemoryManager)
			defaultMemoryManager = new (malloc (sizeof (MemMgr))) MemMgr;
			
		defaultMemMgrMutex.release();
		}

	return defaultMemoryManager;
	}

#ifndef MEMTEST
void* operator new(size_t s) THROWS_BAD_ALLOC
	{
	return getDefaultMemoryManager()->allocate((int) s);
	}
	
void* operator new[](size_t s) THROWS_BAD_ALLOC
	{
	return getDefaultMemoryManager()->allocate((int) s);
	}
	
void* operator new(size_t s, const char *fileName, int line) THROWS_BAD_ALLOC
	{
	return getDefaultMemoryManager()->allocateDebug((int) s, fileName, line);
	}

void* operator new[](size_t s, const char *fileName, int line) THROWS_BAD_ALLOC
	{
	return getDefaultMemoryManager()->allocateDebug((int) s, fileName, line);
	}

void* operator new(size_t s, MemMgr& pool) THROWS_BAD_ALLOC
	{
	return pool.allocate((int) s);
	}
	
void* operator new[](size_t s, MemMgr& pool)THROWS_BAD_ALLOC
	{
	return pool.allocate((int) s);
	}
	
void* operator new(size_t s, MemMgr& pool, char* file, int line) THROWS_BAD_ALLOC
	{
	return pool.allocateDebug((int) s, file, line);
	}
	
void* operator new[](size_t s, MemMgr& pool, char* file, int line)THROWS_BAD_ALLOC
	{
	return pool.allocateDebug((int) s, file, line);
	}

void* operator new(size_t s, MemMgr *pool) THROWS_BAD_ALLOC
	{
	return pool->allocate((int) s);
	}
	
void* operator new[](size_t s, MemMgr *pool)THROWS_BAD_ALLOC
	{
	return pool->allocate((int) s);
	}
	
void* operator new(size_t s, MemMgr *pool, char* file, int line) THROWS_BAD_ALLOC
	{
	return pool->allocateDebug((int) s, file, line);
	}
	
void* operator new[](size_t s, MemMgr *pool, char* file, int line)THROWS_BAD_ALLOC
	{
	return pool->allocateDebug((int) s, file, line);
	}

void operator delete(void* mem) THROWS_NOTHING
	{
	if (mem)
		MemMgr::release(mem);
	}

void operator delete[](void* mem) THROWS_NOTHING
	{
	if (mem)
		MemMgr::release(mem);
	}

#endif // MEMTEST

#ifdef _WIN32
static void* operator new(size_t s, void *p) THROWS_BAD_ALLOC
	{
	return p;
	}

static void* operator new[](size_t s, void *p) THROWS_BAD_ALLOC
	{
	return p;
	}
#endif // _WIN32

MemMgr::MemMgr(int rounding, int cutoff, int minAlloc, bool shared)
{
	init(rounding, cutoff, minAlloc, shared);
}

void MemMgr::init(int rounding, int cutoff, int minAlloc, bool shared)
{
	roundingSize = rounding;
	threshold = cutoff;
	minAllocation = minAlloc;
	int vecSize = (cutoff + rounding) / rounding;
	int l = vecSize * sizeof (void*);
	freeObjects = (MemBlock**) allocRaw (l);
	memset (freeObjects, 0, l);
	bigHunks = NULL;
	smallHunks = NULL;
	freeBlocks.nextLarger = freeBlocks.priorSmaller = &freeBlocks;
	junk.nextLarger = junk.priorSmaller = &junk;
	maxMemory = 0;
	currentMemory = 0;
	blocksAllocated = 0;
	blocksActive = 0;
	threadShared = shared;
}


MemMgr::MemMgr(void* arg1, void* arg2)
{
	init();
}

MemMgr::~MemMgr(void)
{
	releaseRaw (freeObjects);
	freeObjects = NULL;
	
	for (MemSmallHunk *hunk; hunk = smallHunks;)
		{
		smallHunks = hunk->nextHunk;
		releaseRaw (hunk);
		}
		
	{
		for (MemBigHunk *hunk; hunk = bigHunks;)
		{
		bigHunks = hunk->nextHunk;
		releaseRaw (hunk);
		}
	}
}

MemBlock* MemMgr::alloc(int length)
{
	Sync sync (&mutex, "MemMgr::alloc");
	if (threadShared) sync.lock(Exclusive);
	
	// If this is a small block, look for it there
	
	if (length <= threshold)
		{
		int slot = length / roundingSize;
		MemBlock *block;
		
		if (threadShared)
			{
			while (block = freeObjects [slot])
				{
				if (COMPARE_EXCHANGE_POINTER(freeObjects + slot, block, (void *)block->pool))
					{
#ifdef MEM_DEBUG
					if (slot != (-block->length) / roundingSize)
						{
						corrupt ("lenght trashed for block in slot");
						}
#endif
					return block;
					}
				}
			}
		else
			{
			block = freeObjects [slot];
			if (block)
				{
				freeObjects[slot] = (MemBlock *)block->pool;

#ifdef MEM_DEBUG
				if (slot != (-block->length) / roundingSize)
					{
					corrupt ("lenght trashed for block in slot");
					}
#endif
				return block;
				}
			}
		// See if some other hunk has unallocated space to use
		
		MemSmallHunk *hunk;
		
		for (hunk = smallHunks; hunk; hunk = hunk->nextHunk)
			if (length <= hunk->spaceRemaining)
				{
				MemBlock *block = (MemBlock*) hunk->memory;
				hunk->memory += length;
				hunk->spaceRemaining -= length;
				block->length = -length;
				
				return block;
				}
		
		// No good so far.  Time for a new hunk
		
		hunk = (MemSmallHunk*) allocRaw (minAllocation + 16);
		hunk->length = minAllocation;
		hunk->nextHunk = smallHunks;
		smallHunks = hunk;
		
		int l = ROUNDUP(sizeof (MemSmallHunk), sizeof (double));
		block = (MemBlock*) ((UCHAR*) hunk + l);
		hunk->spaceRemaining = minAllocation - length - l;
		hunk->memory = (UCHAR*) block + length;
		block->length = -length;
		
		return block;		
		}
	
	/*
	 *  OK, we've got a "big block" on on hands.  To maximize confusing, the indicated
	 *  length of a free big block is the length of MemHeader plus body, explicitly
	 *  excluding the MemFreeBlock and MemBigHeader fields.
	
                         [MemHeader::length]	
                        	                                  
		                <---- MemBlock ---->
		                
		*--------------*----------*---------*
		| MemBigHeader | MemHeader |  Body  |
		*--------------*----------*---------*
		
		 <---- MemBigObject ----->
		
		*--------------*----------*---------------*
		| MemBigHeader | MemHeader | MemFreeBlock |
		*--------------*----------*---------------*
		
		 <--------------- MemFreeBlock ---------->
	 */
	
	
	MemFreeBlock *freeBlock;
	
	for (freeBlock = freeBlocks.nextLarger; freeBlock != &freeBlocks; freeBlock = freeBlock->nextLarger)
		if (freeBlock->memHeader.length >= length)
			{
			remove (freeBlock);
			MemBlock *block = (MemBlock*) &freeBlock->memHeader;
			
			// Compute length (MemHeader + body) for new free block
			
			int tail = block->length - length;
			
			// If there isn't room to split off a new free block, allocate the whole thing
			
			if (tail < sizeof (MemFreeBlock))
				{
				block->pool = this;
				return block;
				}
			
			// Otherwise, chop up the block
			
			MemBigObject *newBlock = freeBlock;
			freeBlock = (MemFreeBlock*) ((UCHAR*) block + length);
			freeBlock->memHeader.length = tail - sizeof (MemBigObject); 
			block->length = length;
			block->pool = this;
			
			if (freeBlock->next = newBlock->next)
				freeBlock->next->prior = freeBlock;
			
			newBlock->next = freeBlock;
			freeBlock->prior = newBlock;
			freeBlock->memHeader.pool = NULL;		// indicate block is free
			insert (freeBlock);
			
			return block;
			}

			 
	// Didn't find existing space -- allocate new hunk
	
	int hunkLength = sizeof (MemBigHunk) + sizeof(MemBigHeader) + length;
	int freeSpace = 0;
	
	// If the hunk size is sufficient below minAllocation, allocate extra space
	
	if (hunkLength + sizeof(MemBigObject) + threshold < minAllocation)
		{
		hunkLength = minAllocation;
		//freeSpace = hunkLength - 2 * sizeof(MemBigObject) - length;
		freeSpace = hunkLength - sizeof(MemBigHunk) - 2 * sizeof(MemBigHeader) - length;
		}
	
	// Allocate the new hunk
	
	MemBigHunk *hunk = (MemBigHunk*) allocRaw(hunkLength);
	hunk->nextHunk = bigHunks;
	bigHunks = hunk;
	hunk->length = hunkLength;

	// Create the new block
	
	MemBigObject *newBlock = (MemBigObject*) &hunk->blocks;
	newBlock->prior = NULL;
	newBlock->next = NULL;
	
	MemBlock *block = (MemBlock*) &newBlock->memHeader;
	block->pool = this;
	block->length = length;
	
	// If there is space left over, create a free block
	
	if (freeSpace)
		{
		freeBlock = (MemFreeBlock*) ((UCHAR*) block + length);
		freeBlock->memHeader.length = freeSpace;
		freeBlock->memHeader.pool = NULL;
		freeBlock->next = NULL;
		freeBlock->prior = newBlock;
		newBlock->next = freeBlock;
		insert (freeBlock);
		}
	
	return block;		
}

void* MemMgr::allocate(int size)
{
	int length = ROUNDUP(size, roundingSize) + OFFSET(MemBlock*, body) + guardBytes;
	MemBlock *memory = alloc (length);
	memory->pool = this;
	
#ifdef MEM_DEBUG
	memset (&memory->body, INIT_BYTE, size);
	memset (&memory->body + size, GUARD_BYTE, ABS(memory->length) - size - OFFSET(MemBlock*,body));
	memory->fileName = NULL;
	memory->lineNumber = 0;
#endif

	++blocksAllocated;
	++blocksActive;
	
	return &memory->body;
}

void* MemMgr::allocateDebug(int size, const char* fileName, int line)
{
	int length = ROUNDUP(size, roundingSize) + OFFSET(MemBlock*, body) + guardBytes;
	MemBlock *memory = alloc (length);
	memory->pool = this;
	
#ifdef MEM_DEBUG
	memory->fileName = fileName;
	memory->lineNumber = line;
#endif

	memset (&memory->body, INIT_BYTE, size);
	memset (&memory->body + size, GUARD_BYTE, ABS(memory->length) - size - OFFSET(MemBlock*,body));
	++blocksAllocated;
	++blocksActive;
	
	return &memory->body;
}


void MemMgr::release(void* object)
{
	if (object)
		{
		MemBlock *block = (MemBlock*) ((UCHAR*) object - OFFSET(MemBlock*, body));
		((MemMgr*)block->pool)->releaseBlock(block);
		}
}

void MemMgr::releaseBlock(MemBlock *block)
{
	if (!freeObjects)
		return;

	if (block->pool != this)
		corrupt("bad block released");
		
#ifdef MEM_DEBUG
	for (const UCHAR *end = (UCHAR*) block + ABS(block->length), *p = end - guardBytes; p < end;)
		if (*p++ != GUARD_BYTE)
			corrupt ("guard bytes overwritten");
#endif

	--blocksActive;
	int length = block->length;

	
	// If length is negative, this is a small block
	
	if (length < 0)
		{
#ifdef MEM_DEBUG
		memset (&block->body, DELETE_BYTE, -length - OFFSET(MemBlock*, body));
#endif

		if (threadShared)
			for (int slot = -length / roundingSize;;)
			{
			void *next = freeObjects [slot];
			block->pool = (MemMgr*) next;
			if (COMPARE_EXCHANGE_POINTER(freeObjects + slot, next, block))
				return;
			}

		int slot = -length / roundingSize;
		void *next = freeObjects [slot];
		block->pool = (MemMgr*) next;
		freeObjects[slot] = block;
		return;
		}
	
	// OK, this is a large block.  Try recombining with neighbors
	Sync sync (&mutex, "MemMgr::release");
	if (threadShared) sync.lock(Exclusive);

#ifdef MEM_DEBUG
	memset (&block->body, DELETE_BYTE, length - OFFSET(MemBlock*, body));
#endif

	MemFreeBlock *freeBlock = (MemFreeBlock*) ((UCHAR*) block - sizeof (MemBigHeader));
	block->pool = NULL;

	if (freeBlock->next && !freeBlock->next->memHeader.pool)
		{
		MemFreeBlock *next = (MemFreeBlock*) (freeBlock->next);
		remove (next);
		freeBlock->memHeader.length += next->memHeader.length + sizeof (MemBigHeader);
		
		if (freeBlock->next = next->next)
			freeBlock->next->prior = freeBlock;
		}
	
	if (freeBlock->prior && !freeBlock->prior->memHeader.pool)
		{
		MemFreeBlock *prior = (MemFreeBlock*) (freeBlock->prior);
		remove (prior);
		prior->memHeader.length += freeBlock->memHeader.length + sizeof (MemBigHeader);
		
		if (prior->next = freeBlock->next)
			prior->next->prior = prior;
		
		freeBlock = prior;
		}
	
	// If the block has no neighbors, the entire hunk is empty and can be unlinked and
	// released
		
	if (freeBlock->prior == NULL && freeBlock->next == NULL)
		{
		for (MemBigHunk **ptr = &bigHunks, *hunk; hunk = *ptr; ptr = &hunk->nextHunk)
			if (&hunk->blocks == freeBlock)
				{
				*ptr = hunk->nextHunk;
				releaseRaw(hunk);
				return;
				}

		corrupt("can't find big hunk");
		}
	
	insert (freeBlock);
}

void MemMgr::corrupt(const char* text)
{
	throw OSRIMemException(isc_bug_check, isc_arg_end);
}

void* MemMgr::memoryIsExhausted(void)
{
	throw OSRIMemException(isc_virmemexh, isc_arg_end);
}

void MemMgr::remove(MemFreeBlock* block)
{
	// If this is junk, chop it out and be done with it
	
	if (block->memHeader.length < threshold)
		return;
		
	// If we're a twin, take out of the twin list
	
	if (!block->nextLarger)
		{
		block->nextTwin->priorTwin = block->priorTwin;
		block->priorTwin->nextTwin = block->nextTwin;
		return;
		}
	
	// We're in the primary list.  If we have twin, move him in
	
	MemFreeBlock *twin = block->nextTwin;

	if (twin != block)
		{
		block->priorTwin->nextTwin = twin;
		twin->priorTwin = block->priorTwin;
		twin->priorSmaller = block->priorSmaller;
		twin->nextLarger = block->nextLarger;
		twin->priorSmaller->nextLarger = twin;
		twin->nextLarger->priorSmaller = twin;
		return;
		}
	
	// No twins.  Just take the guy out of the list
	
	block->priorSmaller->nextLarger = block->nextLarger;
	block->nextLarger->priorSmaller = block->priorSmaller;
}

void MemMgr::insert(MemFreeBlock* freeBlock)
{
	// If this is junk (too small for pool), stick it in junk
	
	if (freeBlock->memHeader.length < threshold)
		return;
		
	// Start by finding insertion point

	MemFreeBlock *block;
	
	for (block = freeBlocks.nextLarger; 
		 block != &freeBlocks && freeBlock->memHeader.length >= block->memHeader.length;
		 block = block->nextLarger)
		if (block->memHeader.length == freeBlock->memHeader.length)
			{
			// If this is a "twin" (same size block), hang off block
			freeBlock->nextTwin = block->nextTwin;
			freeBlock->nextTwin->priorTwin = freeBlock;
			freeBlock->priorTwin = block;
			block->nextTwin = freeBlock;
			freeBlock->nextLarger = NULL;
			return;
			}
	
	// OK, then, link in after insertion point
	
	freeBlock->nextLarger = block;
	freeBlock->priorSmaller = block->priorSmaller;
	block->priorSmaller->nextLarger = freeBlock;
	block->priorSmaller = freeBlock;
	
	freeBlock->nextTwin = freeBlock->priorTwin = freeBlock;
}

void* MemMgr::allocRaw(int length)
{
	void *memory = malloc (length);
	
	if (!memory)
		memoryIsExhausted();
	
	currentMemory += length;
	
	if (currentMemory > maxMemory)
		maxMemory = currentMemory;
		
	return memory;
}

void MemMgr::debugStop(void)
{
}

void MemMgr::validateFreeList(void)
{
	int len = 0;
	int count = 0;
	MemFreeBlock *block;
	
	for (block = freeBlocks.nextLarger; block != &freeBlocks;  block = block->nextLarger)
		{
		if (block->memHeader.length <= len)
			corrupt ("bad free list\n");
		len = block->memHeader.length;
		++count;
		}
	
	len += 1;
	
	for (block = freeBlocks.priorSmaller; block != &freeBlocks; block = block->priorSmaller)
		{
		if (block->memHeader.length >= len)
			corrupt ("bad free list\n");
		len = block->memHeader.length;
		}

}

void MemMgr::validateBigBlock(MemBigObject* block)
{
	MemBigObject *neighbor;
	
	if (neighbor = block->prior)
		if ((UCHAR*) &neighbor->memHeader + neighbor->memHeader.length != (UCHAR*) block)
			corrupt ("bad neighbors");
	
	if (neighbor = block->next)
		if ((UCHAR*) &block->memHeader + block->memHeader.length != (UCHAR*) neighbor)
			corrupt ("bad neighbors");
}

void MemMgr::releaseRaw(void *block)
{
	free (block);
}

void MemMgr::globalFree(void* block)
{
	release(block);
}

void* MemMgr::calloc(size_t size, int type, const char* fileName, int line)
{
	void *block = allocateDebug((int) size, fileName, line);
	memset (block, 0, size);
	
	return block;
}

void* MemMgr::calloc(size_t size, int type)
{
	void *block = allocate((int) size);
	memset (block, 0, size);
	
	return block;
}

void MemMgr::deallocate(void* block)
{
	release(block);
}

void* MemMgr::allocate(int size, int type)
{
	return allocate(size);
}

void* MemMgr::allocate(int size, int type, const char* fileName, int line)
{
	return allocateDebug(size, fileName, line);
}


void MemMgr::deletePool(MemMgr* pool)
{
	delete pool;
}
