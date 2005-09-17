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
#include "firebird.h"
#include "common.h"
#include "Format.h"

Format::Format(MemoryPool& p, int len) : fmt_desc(len, p, type_fmt)
{
	fmt_length = 0;
	fmt_count = len;
	fmt_version = 0;
}

Format::~Format(void)
{
}

Format* Format::newFmt(MemoryPool& p, int len)
{ 
	return FB_NEW(p) Format(p, len); 
}
