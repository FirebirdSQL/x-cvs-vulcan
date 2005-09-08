/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbaFile.cpp
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

#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "DbaFile.h"
#include "DbaData.h"
#include "DbaRelation.h"
#include "DbaOpenFile.h"

#ifdef WIN_NT
#include <Windows.h>
#include ".\dbafile.h"
#else
#include <stdio.h>
#endif


DbaFile::DbaFile(const char *fileName)
{
	fil_string = fileName;
}

DbaFile::~DbaFile(void)
{
}

void DbaFile::close(void)
{
	if (!fil_desc)
		return;
		
#ifdef WIN_NT
	CloseHandle((HANDLE) fil_desc);
#else
	close((int) desc);
#endif

	fil_desc = NULL;
}

int DbaFile::open(void)
{
#ifdef WIN_NT
	fil_desc = CreateFile(fil_string,
							GENERIC_READ,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL |
							FILE_FLAG_RANDOM_ACCESS,
							0);

	if (fil_desc  == INVALID_HANDLE_VALUE)
		return GetLastError();
#else
	if ((fil_desc = (void*)(IPTR) open(file_name, O_RDONLY)) == -1)
		return errno;
#endif

	return 0;
}

int DbaFile::read(UINT64 offset, int length, void* address)
{
	return 0;
}
