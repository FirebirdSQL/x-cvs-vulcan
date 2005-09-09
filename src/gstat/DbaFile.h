/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbaFile.h
 *	DESCRIPTION:	Database analysis tool
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
 * 2001.08.07 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#ifndef _DBA_FILE_H
#define _DBA_FILE_H

#include "JString.h"

class DbaFile
{
public:
	DbaFile(const char *fileName);
	~DbaFile(void);
	DbaFile*	fil_next;	/* Next file in database */
	ULONG		fil_min_page;			/* Minimum page number in file */
	ULONG		fil_max_page;			/* Maximum page number in file */
	USHORT		fil_fudge;			/* Fudge factor for page relocation */
	void		*fil_desc;
	//USHORT	fil_length;			/* Length of expanded file name */
	//SCHAR		fil_string[1];		/* Expanded file name */
	JString		fil_string;
	void close(void);
	void open(void);
	void read(UINT64 offset, int length, void* address);
};

#endif
