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

#ifndef _TEMPSPACE_H_
#define _TEMPSPACE_H_

class TempSpace
{
public:
	TempSpace(void);
	TempSpace(int length);
	TempSpace(int initialSize, void* initial);
	virtual ~TempSpace(void);
	
	UCHAR	*space;
	int		length;				// effective length (length <= size)

	UCHAR *resize(int newLength, bool copy = false);
	void addByte(UCHAR data);
	void addBytes(int numberBytes, const UCHAR* data);
private:
	UCHAR	*initialSpace;
	int		size;				// allocated space
	int		increment;
};

#endif

