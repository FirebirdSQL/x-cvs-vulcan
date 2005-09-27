/*
 *	PROGRAM:		Firebird Utilities
 *	MODULE:			TempFile.cpp
 *	DESCRIPTION:	Tempory File Management
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "M88K" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.05.11 Nickolay Samofatov - rework temp stuff
 *
 */

#include <string.h>
#include "fbdev.h" 
#include "common.h" 
#include "TempFile.h"
#include "file_params.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>

#else
#include <unistd.h>

#endif

#define FB_TMP_ENV		"FIREBIRD_TMP"


TempFile::TempFile(void)
{
}

TempFile::~TempFile(void)
{
}

JString TempFile::getTempFilename(const char *prototype)
{
	JString directory = getTempDirectory();

	char fileName [MAXPATHLEN];
	char *p = fileName;
	const char *q;
	
	for (q = directory; *q;)
		*p++ = *q++;
	
	if (p [-1] != '/' && p [-1] != '\\')	
		*p++ = '/';
	
	for (q = prototype; *q;)
		*p++ = *q++;
	
	for (q = TEMP_PATTERN; *q;)
		*p++ = *q++;
	
	*p = 0;
		
#ifdef HAVE_MKSTEMP
	mkstemp(fileName);
#else
	if (!mktemp(fileName))
		return "";
#endif

	return fileName;
}

FILE* TempFile::openTempFile(const char* filename, bool autoDelete)
{
	FILE *file = fopen (filename, "w");

#ifndef _WIN32
	if (autoDelete)
		unlink (filename);
#endif

	return file;
}

JString TempFile::getTempDirectory(void)
{
	const TEXT* directory = getenv(FB_TMP_ENV);
	
	if (!directory) 
		{
#ifdef WIN_NT
		char temp_dir[MAXPATHLEN];
		int len = GetTempPath(sizeof(temp_dir), temp_dir);

		if (len && len < sizeof(temp_dir))
			directory = temp_dir;
#else
		directory = getenv("TMP");
#endif
		}
		
	if (!directory || strlen(directory) >= MAXPATHLEN)
		directory = WORKFILE;

	return directory;		
}
