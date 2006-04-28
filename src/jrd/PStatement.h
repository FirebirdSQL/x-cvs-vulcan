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

// PStatement.h: interface for the PStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PSTATEMENT_H__BE6A5E81_18DB_474D_B2FD_AC4A9AE7202F__INCLUDED_)
#define AFX_PSTATEMENT_H__BE6A5E81_18DB_474D_B2FD_AC4A9AE7202F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Connection.h"


class PStatement  
{
public:
	PStatement (PreparedStatement *stmt);
	PStatement();
	virtual ~PStatement();
	void operator =(PreparedStatement *stmt);

	inline PreparedStatement* operator ->()
		{
		return statement;
		}

	PreparedStatement	*statement;
};

#endif // !defined(AFX_PSTATEMENT_H__BE6A5E81_18DB_474D_B2FD_AC4A9AE7202F__INCLUDED_)
