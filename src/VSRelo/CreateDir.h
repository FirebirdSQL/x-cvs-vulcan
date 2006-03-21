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
 *  The Original Code was created by Paul Reeves for IBPhoenix.
 *
 *  Copyright (c) 2006 Paul Reeves
 *
 *  All Rights Reserved.
 */
#if !defined(CREATEDIR_H_)
#define CREATEDIR_H_
 
 
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32
#ifndef _WINDOWS_
#undef ERROR
#include <windows.h>
#endif
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#endif

#include "JString.h"

class CreateDir
{
public:
	CreateDir(const char *filename);
protected:
	char* canonizePath(const char* pathName, char* inBuffer, char* endBuffer);
	
}

#endif // #define CREATEDIR_H_

