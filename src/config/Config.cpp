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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "firebird.h"
#include "common.h"
#include "Config.h"
#include "Configuration.h"
#include "ConfObject.h"
#include "ConfObj.h"
#include "ConfigFile.h"
#include "Args.h"
#include "Provider.h"
#include "AdminException.h"
#include "MemMgr.h"

MemMgr			defaultMemoryManager;

static bool		swTrace;
static bool		swList;
static bool		swVerbose;
static bool		swHelp;
static bool		swInstall;
static char		*databaseName;
static char		*fileName;

static const Switches switches [] =
	{
	"",		NULL,		&databaseName,	"database",	NULL,
	"-t",	&swTrace,	NULL,			NULL,		"Trace configuration file opens",
	"-l",	&swList,	NULL,			NULL,		"List configuration file",
	"-i",	&swInstall,	NULL,			NULL,		"Show installation directory",
	"-v",	&swVerbose, NULL,			NULL,		"Verbose mode",
	"-f",	NULL,		&fileName,		"filename",	"Config file name",
	"-h",	&swHelp,	NULL,			NULL,		"Print this text",
	NULL
	};

static const char* HELP_TEXT =	"Usage: config [-t] [-l] [-f file] [database]\n";

main (int argc, char **argv)
{
	return Config::parse (argc, argv);
}

Config::Config(void)
{
}

Config::~Config(void)
{
}

int Config::parse(int argc, char ** argv)
{
	Args::parse (switches, argc - 1, argv + 1);

	if (swHelp)
		{
		Args::printHelp (HELP_TEXT, switches);
		return 0;
		}

	int flags = 0;
	
	if (swTrace)
		flags |= CONFIG_trace;
		
	if (swList)
		flags |= CONFIG_list | CONFIG_trace;
		
	if (swVerbose)
		flags |= CONFIG_verbose;

	/***
	if (fileName)
		setenv ("VULCAN", fileName, 1);
	***/
	
	try
		{
		ConfigFile *configFile = new ConfigFile (flags);		

		if (swInstall)
			printf ("Installation directory: \"%s\"\n", (const char*) configFile->getInstallDirectory());
				
		if (databaseName)
			{
			printf ("Looking up database name string \"%s\"\n", databaseName);
			JString prior;
			JString dbName = databaseName;
			bool hasProvider = false;
				
			for (int n = 0; !hasProvider && n < 10; ++n)
				{
				ConfObj confObject = configFile->findObject ("database", dbName);
				
				if (!confObject.hasObject())
					break;
					
				const char *name = confObject->getName();
				
				if (!name)
					break;
				
				prior = dbName;
				dbName = confObject->getValue ("filename", (const char*) dbName);
				printf ("  Matches \"%s\", translates to \"%s\"\n", name, (const char*) dbName);

				for (int n = 0;; ++n)
					{
					JString providerName = confObject->getValue (n, "provider");
					if (providerName.IsEmpty())
						break;
					hasProvider = true;
					printf ("    Provider %s\n", (const char*) providerName);
					ConfObj providerObject = configFile->findObject ("provider", providerName);
					Provider *provider = new Provider (providerName, providerObject, "      ");
					delete provider;
					}
				
				if (dbName == prior)
					break;
				}
			}
		
		configFile->release();
		}
	catch (AdminException& exception)
		{
		printf ("%s\n", exception.getText());
		}
	
	return 0;
}

