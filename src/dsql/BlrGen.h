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

#if !defined(AFX_BLRGEN_H__6E8EB18F_4CD4_4C63_A3C9_EEF8E8892C45__INCLUDED_)
#define AFX_BLRGEN_H__6E8EB18F_4CD4_4C63_A3C9_EEF8E8892C45__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Generate.h"

class BlrGen  : public Generate
{
public:
	void blrEndSubstring();
	void blrBeginSubstring (UCHAR verb);
	void appendUCHARs (UCHAR value, int count);
	void appendDynString (UCHAR verb, const TEXT *string);
	void appendNumber (UCHAR verb, int number);
	void appendNumber (int value);
	void appendDynString (UCHAR verb, int length, const TEXT *string);
	BlrGen();
	virtual ~BlrGen();

	int		substringOffset;
	void appendBlrString(const TEXT* string);
	void appendBlrString(int length, const TEXT* string);
	void print(void);
};

#endif // !defined(AFX_BLRGEN_H__6E8EB18F_4CD4_4C63_A3C9_EEF8E8892C45__INCLUDED_)
