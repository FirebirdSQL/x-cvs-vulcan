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

#include "firebird.h"
#include "common.h"
#include "Module.h"
#include "ExternalLibrary.h"
#include "Entrypoint.h"

Module::Module(const char *moduleName, ExternalLibrary *externalLibrary)
{
	name = moduleName;
	library = externalLibrary;
	entrypoints = NULL;
}

Module::~Module(void)
{
	delete library;

	for (Entrypoint *entrypoint; entrypoint = entrypoints;)
		{
		entrypoints = entrypoint->next;
		delete entrypoint;
		}
}

void* Module::findEntrypoint(const char* entrypointName)
{
	Entrypoint *entrypoint;
	
	for (entrypoint = entrypoints; entrypoint; entrypoint = entrypoint->next)
		if (entrypoint->name == entrypointName)
			return entrypoint->entrypoint;
			
	void *entry = library->getEntryPoint(entrypointName);
	
	if (!entry)
		return NULL;
	
	entrypoint = new Entrypoint(name, entry);
	entrypoint->next = entrypoints;
	entrypoints = entrypoint;
	
	return entrypoint->entrypoint;
}
