#ifndef _EXTERNALLIBRARY_H_
#define _EXTERNALLIBRARY_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _WIN32
#include <windows.h>
#endif

#include "JString.h"

typedef void* LibraryHandle;

START_NAMESPACE

class ExternalLibrary
{
public:
	ExternalLibrary(const char* libName);
	ExternalLibrary(const char *name, LibraryHandle libHandle);
	virtual ~ExternalLibrary(void);
	static ExternalLibrary* loadLibrary(const char* libraryName);
	static LibraryHandle load(const char* libraryName);
	void* getEntryPoint(const char* entryPoint);
	static JString getErrorText(void);

	LibraryHandle	libraryHandle;
	JString			libraryName;
};

END_NAMESPACE

#endif
