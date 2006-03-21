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

#include <string.h>
#include "fbdev.h"
#include "common.h"

#include "ModuleManager.h"
#include "Module.h"
#include "Sync.h"
#include "PathName.h"
#include "ExternalLibrary.h"
#include "Entrypoint.h"


ModuleManager::ModuleManager(void)
{
	modules = NULL;
}

ModuleManager::~ModuleManager(void)
{
	for (Module *module; module = modules;)
		{
		modules = module->next;
		delete module;
		}
	
	FOR_OBJECTS(const char*, string, &searchList)
		delete []string;
	END_FOR;
}

Module* ModuleManager::findModule(const char* name)
{
	Sync sync (&syncObject, "ModuleManager::findModule");
	sync.lock(Shared);
	Module *module;
	
	for (module = modules; module; module = module->next)
		if (module->name == name)
			return module;
	
	sync.unlock();
	sync.lock(Exclusive);
	
	for (module = modules; module; module = module->next)
		if (module->name == name)
			return module;
	
	FOR_OBJECTS(const char*, string, &searchList)
		char path[MAXPATHLEN];
		PathName::merge(name, string, sizeof (path), path);
		ExternalLibrary *library = ExternalLibrary::loadLibrary(path);
		
		if (library)
			{
			module = new Module(name, library);
			module->next = modules;
			modules = module;
			
			return module;
			}
	END_FOR;

	ExternalLibrary *library = ExternalLibrary::loadLibrary(name);
	
	if (library)
		{
		module = new Module(name, library);
		module->next = modules;
		modules = module;
		}

	return module;	
}

void ModuleManager::addSearchPath(const char* path)
{
	char *string = new char [strlen(path) + 1];
	strcpy (string, path);
	searchList.append(string);
}
