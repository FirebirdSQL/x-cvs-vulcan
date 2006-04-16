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

#ifndef _QSORT_H_
#define _QSORT_H_

#ifndef SORT_ASSERT
#define SORT_ASSERT(boolean)
#endif

template <typename T, int stackSize=256>
class QSort
{
public:
	virtual int	compare(T rec1, T rec2) = 0;
	
	void sort()
	{
		int iteration = 0;
		int i, j, r, stack = 0;
		int low [stackSize];
		int high [stackSize];
		T	temp;

		if (size > 2)
			{
			low [stack] = 0;
			high [stack++] = size - 1;
			}

		while (stack > 0)
			{
			T key;
			/***
			if (++iteration > 10)
				{
				Debug.print ("punting");
				return;
				}
			***/
			r = low [--stack];
			j = high [stack];
			//Debug.print ("sifting " + r + " thru " + j + ": " + range (r, j));
			int interval = j - r;
			key = records [r];
			//Debug.print (" key", key);
			int limit = j;
			i = r + 1;
			for (;;)
				{
				while (compare (records [i], key) <= 0 && i < limit)
					++i;
				while (compare (records [j], key) >= 0 && j > r)
					--j;
				//Debug.print ("  i " + i, records [i]);
				//Debug.print ("  j " + j, records [j]);
				if (i >= j)
					break;
				temp = records [i];
				records [i] = records [j];
				records [j] = temp;
				}
			i = high [stack];
			records [r] = records [j];
			records [j] = key;
			//Debug.print (" midpoint " + j + ": " +  range (r, i));
			const int loSize = (j - 1) - r;
			const int hiSize = i - (j + 1);
			if (loSize > hiSize) 
				{
				if (loSize >= 2)
					{
					low [stack] = r;
					SORT_ASSERT (stack < stackSize);
					high [stack++] = j - 1;
					}
				if (hiSize >= 2)
					{
					low [stack] = j + 1;
					SORT_ASSERT (stack < stackSize);
					high [stack++] = i;
					}
				} 
			else
				{
				if (hiSize >= 2)
					{
					low [stack] = j + 1;
					SORT_ASSERT (stack < stackSize);
					high [stack++] = i;
					}
				if (loSize >= 2)
					{
					low [stack] = r;
					SORT_ASSERT (stack < stackSize);
					high [stack++] = j - 1;
					}
				}
			}

		for (int n = 1; n < size; ++n)
			if (compare (records [n - 1], records [n]) > 0)
			{
				//Debug.print ("Flipping");
				temp = records [n - 1];
				records [n - 1] = records [n];
				records [n] = temp;
			}
	}
	
	T	*records;
	int	size;
};

#endif
