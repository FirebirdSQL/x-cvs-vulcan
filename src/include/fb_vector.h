/*
 *  fb_vector.h
 *  firebird_test
 *
 *  Created by john on Fri Dec 14 2001.
 *  Copyright (c) 2001 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef FB_VECTOR_H
#define FB_VECTOR_H
 
#include "../include/fb_types.h"
#include "../common/classes/alloc.h"

#ifdef MEMMGR
#include "MemoryManager.h"
/***
class MemMgr;
#ifndef MemoryPool
#define MemoryPool	::MemMgr
#endif
***/
#endif

#include <vector>

namespace firebird
{
	template<class T>
	class vector : public std::vector<T, firebird::allocator<T> >
	{
	public:
		vector(int len) : std::vector<T, firebird::allocator<T> >(len) {}
		vector(int len, MemoryPool &p, SSHORT type = 0)
			: std::vector<T, firebird::allocator<T> >(len, T(),
				firebird::allocator<T>(p, type)) {}
		vector(MemoryPool &p, SSHORT type = 0)
			: std::vector<T, firebird::allocator<T> >(
					firebird::allocator<T>(p, type) ) {}
	};
};

#endif	// FB_VECTOR_H

