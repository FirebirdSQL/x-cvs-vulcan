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
 *  The Original Code was created by Ann W. Harrison for IBPhoenix.
 *
 *  Copyright (c) 2004 Ann W. Harrison
 *  All Rights Reserved.
 */

// Procedure.h definitions for the procedure class

#ifndef _ProcManager_H_
#define _ProcManager_H_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"
#include "SyncObject.h"

class	Database;
class	Procedure;
struct	tdbb;

class ProcManager
{
public:
	ProcManager(Database *dbb);
	~ProcManager(void);

	inline Database* getDatabase ()
		{return database;};
		
	inline void setDatabase (Database *db)
		{database = db;};
		
	inline Procedure* findFirst()
		{return procList;};
	
	void	addProcedure (Procedure *procedure);
	Procedure*	findProcedure (thread_db* tdbb, JString name, bool noscan);
	Procedure*	findProcedure (thread_db* tdbb, int id);
	Procedure*	findProcedure (thread_db* tdbb, const TEXT *name, bool noscan);
	Procedure*	findKnownProcedure (thread_db* tdbb, int id);
	Procedure*	findKnownProcedure (const TEXT *name);

	void	dropProcedure (int id);
	void	dropProcedure (const TEXT *name);
	void	validateCache (thread_db* tdbb);
	bool	clearCache (thread_db* tdbb, Procedure *proc);
	bool	procedureInUse (thread_db* tdbb, Procedure *procedure);

protected:

	Database	*database;
	Procedure	*procList;
	SyncObject	syncObject;
	
	void	adjustDependencies (Procedure *procedure);

public:
	Procedure* findObsoleteProcedure(thread_db* tdbb, int id);
	void remove(Procedure* procedure);
	void purgeDependencies(Procedure* procedure);
};
#endif

