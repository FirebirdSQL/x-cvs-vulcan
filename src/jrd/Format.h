/*
 *	PROGRAM:	JRD access method
 *	MODULE:		Format.h
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

#ifndef _FORMAT_H
#define _FORMAT_H

//#include "../jrd/jrd_blks.h"
//#include "../include/fb_blk.h"
//#include "../include/fb_vector.h"
#include "../jrd/dsc.h"


class Format
{
public:
	Format(MemoryPool *p, const Format* format);
	Format(MemoryPool *p, int len);
	~Format(void);

	USHORT		fmt_length;
	USHORT		fmt_count;
	USHORT		fmt_version;
	MemoryPool	*fmt_pool;
	dsc			*fmt_desc;
	
	static Format* newFmt(MemoryPool& p, int len);

	/***
	firebird::vector<dsc> fmt_desc;
	typedef firebird::vector<dsc>::iterator fmt_desc_iterator;
	typedef firebird::vector<dsc>::const_iterator fmt_desc_const_iterator;
	***/
	void resize(int newSize);
	void alloc(int size);
	Format(int count);
};

#endif
