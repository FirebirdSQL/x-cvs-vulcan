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

// JVector.h: interface for the JVector class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_JVECTOR_H__704DE82B_945E_48FB_991D_5D8E2E0C42B9__INCLUDED_)
#define AFX_JVECTOR_H__704DE82B_945E_48FB_991D_5D8E2E0C42B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



class JVector  
{
public:
	JVector();
	virtual ~JVector();

	JVector(int initialSize);
	
	int		allocated;
	int		count;
	void	**vector;
	void init(int initialSize);
	void extend(int newSize);
	void* findElement(int index);
	void* operator [](int index);
	void append(void* object);
	void setElement(int index, void* object);
};

#endif // !defined(AFX_JVECTOR_H__704DE82B_945E_48FB_991D_5D8E2E0C42B9__INCLUDED_)
