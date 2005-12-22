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
#include "fbdev.h"
#include "common.h"
#include "DbaFile.h"
#include "DbaData.h"
#include "DbaRelation.h"
#include "OSRIException.h"
#include "iberror.h"

#ifdef WIN_NT
#include <Windows.h>
#else
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
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
	::close(fil_desc);
#endif

	fil_desc = 0;
}

void DbaFile::open(void)
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
		// msg 29: Can't open database file %s 
		throw OSRIException(isc_gstat_open_err, isc_arg_string, (const char*) fil_string, 0);	
#else
	if ((fil_desc = ::open(fil_string, O_RDONLY)) == -1)
		throw OSRIException(isc_gstat_open_err, isc_arg_string, (const char*) fil_string, 0);
#endif
}

void DbaFile::read(UINT64 offset, int length, void* address)
{
#ifdef WIN_NT
	LONG low = (LONG) offset;
	LONG high = (LONG) (offset >> 32);
	
	DWORD ret = SetFilePointer(fil_desc, low, &high, FILE_BEGIN);
	
	if (ret == INVALID_SET_FILE_POINTER)
		{
		int lastError = GetLastError();
		
		if (lastError != NO_ERROR)
			// msg 30: Can't read a database page 
			throw OSRIException(GSTAT_CODE(30), isc_arg_win32, lastError, 0);
		}

	if (!ReadFile(fil_desc, address, length, &ret, NULL))
			// msg 30: Can't read a database page 
			throw OSRIException(GSTAT_CODE(30), 0);
#else
	if (lseek (fil_desc, offset, 0) < 0) 
		throw OSRIException(GSTAT_CODE(30), isc_arg_unix, errno, 0);
	
	int l = ::read(fil_desc, address, length);
	
	if (l <= 0)
		throw OSRIException(GSTAT_CODE(30), isc_arg_unix, errno, 0);
#endif
	
}
