/*
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
 *  The Original Code was created by James K. Lowden.
 *
 *  Copyright (c) 2004 James K. Lowden <jklowden@schemamania.org>
 */

#ifndef _SVECTOR_H_
#define _SVECTOR_H_

//  On Tue, 20 Jul 2004 Jim Starkey <jas@netfrastructure.com> wrote:
//  I'd like to spec out the requirements for a suitable Vector template 
//  (probably called something exotic like "Vector").  Here are the things I 
//  know we need:
//  
//     1. A default constructor that doesn't allocate anything
//     2. A constructor to set an initial capacity
//     3. A method to change the capacity
//     4. A method to access an element
//     5. A method to set an element (out of bounds is an error -- no
//        automatic extension)
//     6. A method to get capacity
//  
//  Things that I don't think we need include:
//  
//     1. Automatic extension (this should be an error)
//     2. A mechanism to squish in a new element (not needed)
//     3. A mechanism to sort the vector
//     4. Any sort of iterator (all vectors start with zero)
//     5. Copy constructors
//     6. An auto-insert (assign to next unused slot)
//  
//  Things I consider up in the air:
//  
//     1. A method to get a const vector pointer (yes, Virginia, a const
//        vector pointer)
//     2. An index operator
//     3. A high water mark (static?  dynamic?)

#include "OSRIBugcheck.h"

template <typename Type> 
class SVector
{
public:
	typedef unsigned int size_type;

protected:
	Type *buffer;
	size_type length, true_length;
	
	// Prevent copying; see Jim Starkey's comments above. 
	 
	SVector( const SVector& );
	void operator=( const SVector& );
	
public:
	// Allocate nothing by default
	SVector() : buffer(0), length(0), true_length(0) {}

	// Allocate N elements
	SVector(size_type n) : length(n), true_length(n) 
	{ 
		try 
			{ 
			buffer = new Type[n]; 
			}
		catch(...) 
			{
			throw OSRIException(osriMemExhausted);
			}
	}

	virtual ~SVector() { delete[] buffer; }
	
	/*
	 * Access to an element is by the at() function or by operator[].
	 * Both return a reference (const or not, as appropriate).  
	 * To modify the contents of element i in SVector<char> v: v[i] = 'a';
	 */
	// by function
	
	Type& at(size_type n) { return SVector::buffer[n]; }
	const Type& at(size_type n) const { return SVector::buffer[n]; }
	
	// by index operator
	Type& operator[](size_type n)
		{
		if( n < length ) 
			return buffer[n];
			
		throw OSRIBugcheck("vector index error");
		}
		
	const Type& operator[](size_type n) const
		{
		if( n < length ) 
			return buffer[n];
			
		throw OSRIBugcheck("vector index error");
		}
	
	// Get current size 
	size_type size() const { return length; }

	// Get maximum current capacity, cf. resize()
	size_type capacity() const { return true_length; }
	
	// Get a const vector pointer
	const Type* pointer() const { return buffer; }
	
	// check size, resize if necessary
	
	void checkSize(int check, int newSize)
		{
		if (check < length)
			return;
		
		resize(newSize);
		}
		
	// Change the capacity
	SVector&  resize(size_type n, Type e = Type())
		{
		if( n == 0 ) 
			{
			delete[] buffer;
			buffer = NULL;
			length = true_length = 0;
			return *this;
			}
		
		if( n <= true_length ) 
			{
			if (n > length)
				memset(buffer + length, 0, sizeof(Type) * (n - length));
				
			length = n;
			return *this;
			}

		Type *new_buffer;
		try 
			{ 
			new_buffer = new Type[n]; 
			}
		catch(...) 
			{
			throw OSRIException(osriMemExhausted);
			}

		// memberwise copy, should use std::copy()
		for( size_type i=0; i < length; i++ ) 
			new_buffer[i] = buffer[i];
		
		memset(new_buffer + length, 0, sizeof(Type) * (n - length));
		delete[] buffer;
		buffer = new_buffer;
		length = true_length = n;
		
		return *this;
		}
};

#endif //  _SVECTOR_H_

