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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef _MEMMGR_H_
#define _MEMMGR_H_

#include "Mutex.h"
#include "SyncObject.h"

#ifndef MEMORY_EXCEPTION
#define MEMORY_EXCEPTION	OSRIMemException
#endif

#ifndef MEM_DEBUG
#define MEM_DEBUG
#endif

#ifdef MEMTEST
#undef MEM_DEBUG
#endif

static const int defaultRounding = 8;
static const int defaultCutoff = 4096;
static const int defaultAllocation = 65536;

START_NAMESPACE

class MemMgr;

class MemHeader 
{
public:
	volatile MemMgr	*pool;
	INT32			length;
#ifdef MEM_DEBUG
	INT32		lineNumber;
	const char	*fileName;
#endif
};
	
class MemBlock : public MemHeader 
{
public:
	UCHAR		body;
};

class MemBigObject;

class MemBigHeader
{
public:
	MemBigObject	*next;
	MemBigObject	*prior;
};

class MemBigObject : public MemBigHeader
{
public:
	MemHeader		memHeader;
};


class MemFreeBlock : public MemBigObject 
{
public:
	MemFreeBlock	*nextLarger;
	MemFreeBlock	*priorSmaller;
	MemFreeBlock	*nextTwin;
	MemFreeBlock	*priorTwin;
};


class MemSmallHunk 
{
public:
	MemSmallHunk	*nextHunk;
	int				length;
	UCHAR			*memory;
	int				spaceRemaining;
};
	
class MemBigHunk 
{
public:
	MemBigHunk		*nextHunk;
	int				length;
	MemBigHeader	blocks;
};

class MemMgr
{
public:
	MemMgr(int rounding=defaultRounding, int cutoff=defaultCutoff, 
		   int minAllocation=defaultAllocation, bool shared=true);
	~MemMgr(void);
	
	int				roundingSize;
	int				threshold;
	int				minAllocation;
	//int			headerSize;
	MemBlock		**freeObjects;
	MemBigHunk		*bigHunks;
	MemSmallHunk	*smallHunks;
	MemFreeBlock	freeBlocks;
	MemFreeBlock	junk;
	Mutex			mutex;		// Win32 critical regions are faster than SyncObject
	//SyncObject	mutex;		// SyncObject is faster than pthreads
	int				maxMemory;
	int				currentMemory;
	int				blocksAllocated;
	int				blocksActive;
	bool			threadShared;		// Shared across threads, requires locking

protected:
	MemBlock* alloc(int size);
	
public:
	void* allocate(int size);
	void* allocateDebug(int size, const char* fileName, int line);
	void releaseBlock(MemBlock *block);
protected:
	void corrupt(const char* text);
public:
	virtual void* memoryIsExhausted(void);
	void remove(MemFreeBlock* block);
	void insert(MemFreeBlock* block);
	void* allocRaw(int length);
	void debugStop(void);
	void validateFreeList(void);
	void validateBigBlock(MemBigObject* block);
	static void release(void* block);
	void releaseRaw(void *block);
	MemMgr(void* arg1, void* arg2);
	void init (int rounding=defaultRounding, int cutoff=defaultCutoff, 
		   int minAllocation=defaultAllocation, bool shared=true);
	
	// crap for backward compatility with MemoryPool
	
	static void deletePool(MemMgr* pool);
	static void globalFree(void* block);
	void* calloc(size_t size, int type, const char* fileName, int line);
	void* calloc(size_t size, int type=0);
	static void deallocate(void* block);
	void* allocate(int size, int type);
	void* allocate(int size, int type, const char* fileName, int line);
};

END_NAMESPACE

#endif

