#ifndef FB_LIST_H
#define FB_LIST_H
 
#include "../common/classes/alloc.h"

#include <list>

START_NAMESPACE

	template <class T>
	class list : public std::list<T, Firebird::allocator<T> >
	{
	};

END_NAMESPACE

#endif	// FB_LIST_H
