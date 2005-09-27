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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#include <string.h>
#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "Provider.h"
#include "ConfObject.h"
#include "ExternalLibrary.h"
#include "Subsystem.h"

#define GET_SUBSYSTEMS	"getSubsystems"

typedef Subsystem** (GetSubsystems)();

Provider::Provider(JString providerName, ConfObject *config, const char *prefix)
{
	name = providerName;
	configuration = config;
	subsystems = NULL;
	
	if (configuration)
		for (int n = 0;; ++n)
			{
			JString libraryName = configuration->getValue (n, "library");
			if (libraryName.IsEmpty())
				break;
			ExternalLibrary *externalLibrary = ExternalLibrary::loadLibrary (libraryName);
			if (externalLibrary)
				{
				GetSubsystems* fn = (GetSubsystems*)externalLibrary->getEntryPoint (GET_SUBSYSTEMS);
				if (fn)
					subsystems = (fn)();
				delete externalLibrary;
				}
			if (prefix)
				printf ("%sLibrary \"%s\": %s\n", prefix, (const char*) libraryName,
						(subsystems) ? "succeeded" : (const char*) ExternalLibrary::getErrorText());
			if (subsystems)
				break;
			}
}

Provider::~Provider(void)
{
}


int Provider::shutdownConnections(int type, int milliseconds)
{
	int count = 0;
	
	if (subsystems)
		for (Subsystem **subsystem = subsystems; *subsystem; ++subsystem)
			count += (*subsystem)->shutdownConnections (type, milliseconds);
	
	return count;
}
