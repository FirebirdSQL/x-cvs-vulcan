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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

//
//////////////////////////////////////////////////////////////////////

#ifndef _CharSetManager_H_
#define _CharSetManager_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SyncObject.h"
#include "SVector.h"

class Database;
class CharSetContainer;
class Collation;
struct thread_db;

class CharSetManager
{
public:
	CharSetManager(Database *dbb);
	~CharSetManager(void);
	
	Database			*database;
	CharSetContainer	*charSets;
	Collation			*collations;
	SyncObject			syncObject;
	SVector<CharSetContainer*>	charSetVector;
	
	CharSetContainer*	findCharset(thread_db* tdbb, int ttype);

	CharSetContainer* findCharset(thread_db* tdbb, const char* name);
	CharSetContainer* loadCharset(thread_db* tdbb, int ttype);
	CharSetContainer* findCollation(thread_db* tdbb, const char* name);
};

#endif

