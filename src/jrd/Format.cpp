/*
 *	PROGRAM:	JRD access method
 *	MODULE:		Format.cpp
 *	DESCRIPTION:	Definitions associated with Format class
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "Format.h"

Format::Format(MemoryPool *p, int count) //: fmt_desc(len, p, type_fmt)
{
	fmt_pool = p;
	fmt_length = 0;
	fmt_version = 0;
	alloc(count);
}


Format::Format(int count)
{
	fmt_pool = NULL;
	fmt_length = 0;
	fmt_version = 0;
	alloc(count);
}

Format::Format(MemoryPool *p, const Format* format)
{
	fmt_pool = p;
	fmt_length = format->fmt_length;
	fmt_version = format->fmt_version;
	alloc(format->fmt_count);
	memcpy(fmt_desc, format->fmt_desc, fmt_count * sizeof(fmt_desc[0]));
}

Format::~Format(void)
{
	delete [] fmt_desc;
}

Format* Format::newFmt(MemoryPool& p, int len)
{ 
	return FB_NEW(p) Format(&p, len); 
}

void Format::resize(int newSize)
{
	int n = MIN(fmt_count, newSize);
	dsc *oldVector = fmt_desc;
	alloc(newSize);
	memcpy(fmt_desc, oldVector, n * sizeof(fmt_desc[0]));
	delete [] fmt_desc;
}

void Format::alloc(int size)
{
	fmt_count = size;
	
	if (fmt_pool)
		fmt_desc = new (fmt_pool) dsc[fmt_count];
	else
		fmt_desc = new dsc[fmt_count];
}
