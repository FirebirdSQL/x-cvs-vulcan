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

// RSet.h: interface for the RSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSET_H__3E3DE313_DB93_4640_95CE_41D339395A2F__INCLUDED_)
#define AFX_RSET_H__3E3DE313_DB93_4640_95CE_41D339395A2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "fbdev.h"
#include "Connection.h"


class RSet  
{
public:
	RSet (ResultSet *results);
	RSet();
	virtual ~RSet();
	void operator =(ResultSet *results);

	inline ResultSet* operator ->() { return resultSet; }
	inline operator ResultSet* () { return resultSet; }

	ResultSet	*resultSet;
};

#endif // !defined(AFX_RSET_H__3E3DE313_DB93_4640_95CE_41D339395A2F__INCLUDED_)
