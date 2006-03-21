/*
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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#include "fbdev.h"
#include "common.h"
#include "DatabaseManager.h"
#include "Database.h"
#include "Sync.h"

DatabaseManager::DatabaseManager(void)
{
	databases = NULL;
}

DatabaseManager::~DatabaseManager(void)
{
}

Database* DatabaseManager::findDatabase(const char* expandedFilename)
{
	for (Database *dbb = databases; dbb; dbb = dbb->dbb_next)
		if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use)) && dbb->isFilename (expandedFilename))
			{
			dbb->addRef();
			return dbb;
			}
	
	return NULL;
}

#ifndef SHARED_CACHE
int DatabaseManager::countDatabase(const char* expandedFilename)
{
	Sync sync (&syncObject, "DatabaseManager::getDatabase");
	sync.lock (Shared);

	int count = 0;
	for (Database *dbb = databases; dbb; dbb = dbb->dbb_next)
		if (!(dbb->dbb_flags & (DBB_bugcheck | DBB_not_in_use)) && dbb->isFilename (expandedFilename))
			{
			count++;
			}
	
	return count;
}
#endif


Database* DatabaseManager::getDatabase(const char* expandedFilename, ConfObject *configObject)
{
	Database *dbb;
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "DatabaseManager::getDatabase");
	sync.lock (Shared);
	
	dbb = findDatabase (expandedFilename);
	
	if (dbb)
		{
		dbb->addRef();
		sync.unlock();
		dbb->syncExistence.lock(NULL, Shared);
		return dbb;
		}
	
	sync.unlock();
	sync.lock (Exclusive);
	
	if (dbb = findDatabase (expandedFilename))
		{
		dbb->addRef();
		sync.unlock();
		dbb->syncExistence.lock(NULL, Shared);
		return dbb;
		}
#else
	
	Sync sync (&syncObject, "DatabaseManager::getDatabase");
	sync.lock (Exclusive);
#endif

	dbb = new Database (expandedFilename, configObject);
	dbb->dbb_next = databases;
	databases = dbb;
#ifdef SHARED_CACHE
	dbb->syncExistence.lock(NULL, Exclusive);
#else
	dbb->databaseManager = this;
#endif
	
	return dbb;
}

/***
Database* DatabaseManager::createDatabase(const char* expandedFilename, ConfObject* configObject)
{
	Sync sync (&syncObject, "DatabaseManager::getDatabase");
	sync.lock (Exclusive);
	
	Database *dbb = findDatabase (expandedFilename);
	
	if (dbb)
		{
		dbb->release();
		return NULL;
		}
		
	dbb = new Database (expandedFilename, configObject);
	dbb->dbb_next = databases;
	databases = dbb;
	
	return dbb;
}
***/

void DatabaseManager::remove(Database* database)
{
	Sync sync (&syncObject, "DatabaseManager::remove");
	sync.lock (Exclusive);
	
	for (Database **ptr = &databases; *ptr; ptr = &(*ptr)->dbb_next)
		if (*ptr == database)
			{
			*ptr = database->dbb_next;
			break;
			}
}
