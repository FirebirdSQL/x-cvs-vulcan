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

//
//////////////////////////////////////////////////////////////////////

#ifndef _Collation_H_
#define _Collation_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"
#include "CsConvertArray.h"

class CharSetContainer;
struct texttype;

class Collation : public CharSetContainer
{
public:
	Collation(const char *collationName, int charSet, int collation, texttype *type);
	virtual ~Collation(void);
	
	Collation		*next;
	JString			name;
	int				charSetId;
	int				collationId;
	texttype		*textType;
	
	virtual bool isNamed(const char* name);
	virtual int getBytesPerChar(void);
	virtual int getTType(void);
	virtual int getCharsetId(void);
	virtual int getCollationId(void);
};

#endif
