/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by [Initial Developer's Name]
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2003 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

#include <stdio.h>
#include <string.h>
#include "firebird.h"
#include "common.h"
#include "MemMgr.h"
#include "ExternalLibrary.h"

#ifdef _WIN32
#include <windows.h>
static const char* defaultExtension =	".dll";
#else
#include <dlfcn.h>
static const char* defaultExtension =	".so";
#endif

ExternalLibrary::ExternalLibrary(const char *name, LibraryHandle libHandle)
{
	libraryName = name;
	libraryHandle = libHandle;
}

ExternalLibrary::ExternalLibrary(const char* name)
{
	libraryName = name;
}

ExternalLibrary::~ExternalLibrary(void)
{
}

ExternalLibrary* ExternalLibrary::loadLibrary(const char* libraryName)
{
	char fileName [256];
	strcpy (fileName, libraryName);
	LibraryHandle handle = load (fileName);

	if (!handle)
		{
		strcat (fileName, defaultExtension);
		handle = load (fileName);
		}
	
	if (!handle)
		return NULL;
	
	return new ExternalLibrary (fileName, handle);
}

LibraryHandle ExternalLibrary::load(const char* libraryName)
{
#ifdef _WIN32
	return LoadLibrary (libraryName);
#else
	return dlopen (libraryName, RTLD_LAZY | RTLD_GLOBAL);
#endif
}

JString ExternalLibrary::getErrorText(void)
{
	JString diagnostic;

#ifdef _WIN32
	diagnostic.Format ("windows error %d", GetLastError());
#else
	diagnostic = dlerror();
#endif

	return diagnostic;
}

void* ExternalLibrary::getEntryPoint(const char* entryPoint)
{
#ifdef _WIN32
	return GetProcAddress ((HINSTANCE) libraryHandle, entryPoint);
#else
	return dlsym (libraryHandle, entryPoint);
#endif
}
