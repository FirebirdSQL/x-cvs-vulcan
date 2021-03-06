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

#ifndef _MODULE_H_
#define _MODULE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "JString.h"

CLASS (ExternalLibrary);
CLASS (Entrypoint);

START_NAMESPACE

class Module
{
public:
	Module(const char *moduleName, ExternalLibrary *externalLibrary);
	virtual ~Module(void);
	void* findEntrypoint(const char* entrypointName);
	
	JString			name;
	Module			*next;
	ExternalLibrary	*library;
	Entrypoint		*entrypoints;
};

END_NAMESPACE

#endif
