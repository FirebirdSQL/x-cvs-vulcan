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

// BlrGen.h: interface for the BlrGen class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PBGEN_H
#define PBGEN_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Generate.h"

START_NAMESPACE

class PBGen : public Generate
{
public:
	PBGen(void);
	virtual ~PBGen(void);
	void putParameter(UCHAR parameter, int value);
	void putParameter(UCHAR parameter, const TEXT* string);
	void putParameter(UCHAR parameter, int length, const TEXT* string);
	void putParameter(UCHAR parameter);
	PBGen(UCHAR version);
	void putParameter(UCHAR parameter, int length, const UCHAR* data);
};

END_NAMESPACE

#endif

