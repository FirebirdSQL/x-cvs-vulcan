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

#ifndef _MemoryManager_h_
#define _MemoryManager_h_

#ifndef MemoryPool
#define MemoryPool	NAMESPACE::MemMgr
#endif

#ifndef MEMORY_EXCEPTION
#define MEMORY_EXCEPTION	OSRIMemException
#endif

#define getDefaultMemoryPool	::getDefaultMemoryManager

#ifdef DEBUG_GDS_ALLOC
#define FB_NEW(pool) new(pool,__FILE__,__LINE__)
#define FB_NEW_RPT(pool,count) new(pool,count,__FILE__,__LINE__)
#else
#define FB_NEW(pool) new(pool)
#define FB_NEW_RPT(pool,count) new(pool,count)
#endif // DEBUG_GDS_ALLOC

#ifdef _WIN32
#define THROWS_BAD_ALLOC
#define THROWS_NOTHING

#else

#include <new>
/***
namespace std
{
	class bad_alloc {};
};
***/

#define THROWS_BAD_ALLOC	throw (std::bad_alloc)
#define THROWS_NOTHING		throw()
#endif

namespace NAMESPACE
{
class MemMgr;
};

MemMgr* getDefaultMemoryManager();
void* operator new(size_t s, const char *fileName, int line) THROWS_BAD_ALLOC;
void* operator new[](size_t s, const char *fileName, int line) THROWS_BAD_ALLOC;
void* operator new(size_t s, MemMgr& pool) THROWS_BAD_ALLOC;
void* operator new[](size_t s, MemMgr& pool) THROWS_BAD_ALLOC;
void* operator new(size_t s, MemMgr& pool, char* file, int line) THROWS_BAD_ALLOC;
void* operator new[](size_t s, MemMgr& pool, char* file, int line) THROWS_BAD_ALLOC;
void* operator new(size_t s) THROWS_BAD_ALLOC;
void* operator new[](size_t s) THROWS_BAD_ALLOC;

#ifdef _WIN32
void* operator new(size_t s, void *p) THROWS_BAD_ALLOC;
void* operator new[](size_t s, void *p) THROWS_BAD_ALLOC;
#endif // _WIN32

void operator delete(void* mem) THROWS_NOTHING;
void operator delete[](void* mem) THROWS_NOTHING;


#endif

