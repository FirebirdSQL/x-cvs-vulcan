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

#ifndef _Procedure_H_
#define _Procedure_H_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

class dsql_sym;
class fmt;
class ProcParam;
class lck;
class ProcManager;
class Database;
class Request;
class Csb;
struct jrd_nod;
struct bid;

const char PRC_scanned = 1,		/* Field expressions scanned */
	PRC_system = 2,
	PRC_obsolete = 4,			/* Procedure known gonzo */
	PRC_being_scanned = 8,		/* New procedure needs dependencies during scan */
	PRC_blocking = 16,			/* Blocking someone from dropping procedure */
	PRC_create = 32,			/* Newly created */
	PRC_being_altered = 64,		/* Procedure is getting altered */
	PRC_check_existence	= 128;	/* Existence lock released */

const char  MAX_PROC_ALTER = 64; /* No. of times an in-cache procedure can be altered */

class Procedure
{
public:
	// constructors & destructor
	Procedure(Database *dbb, const TEXT *name, const TEXT *owner, int id);
	virtual ~Procedure(void);

	// Procedure Name
	inline JString findName()
		{return procName;};
	inline void setName (JString name)
		{procName = name;};
	inline bool hasName()
		{ return procName.IsEmpty(); };
		
	inline bool isNamed (const TEXT * name)
		{ return procName == name; };

	//Input & Output Parameters
	inline int	findInputCount()
		{return procInputCount;};
	inline void setInputCount (int count)
		{procInputCount = count;};
	inline ProcParam * findInputParameters()
		{return procInputParams;};
	inline int findOutputCount()
		{return procOutputCount;};
	inline void setOutputCount (int count)
		{procOutputCount = count;};
	inline ProcParam * findOutputParameters()
		{return procOutputParams;};
	inline bool hasOutputParameters()
		{return procOutputParams != NULL;};
	inline bool hasInputParameters()
		{return procInputParams != NULL;};

	void setInputParameter (ProcParam *parameter);
	void setOutputParameter (ProcParam * parameter);
	int findOutputParameter (const TEXT * name);
	ProcParam *findOutputParameter (int id);
	ProcParam *findInputParameter (int id);
	ProcParam *findParameterById (ProcParam *param, int id);
	int findInputParameter (const TEXT * name);
	int findParameter (ProcParam *param, const TEXT *name);


	//Owner
	inline JString findOwner()
		{return procOwner;};
	inline void setOwner (JString owner)
		{procOwner = owner;};
	
	//Id
	inline int findId()
		{return procId;};
	inline void setId (int id)
		{procId = id;};

	//flag stuff
	inline int findFlags ()
		{return procFlags;};
	inline void setFlags (int flags)
		{procFlags = flags;};
	inline void addFlags(int flag)
		{procFlags |= flag;};
	inline void clearFlags (int flag)
		{procFlags &= ~flag;};
	inline bool checkFlags (int flag)
		{return procFlags & flag;};
		
	inline bool checkActive (bool noscan)
		{ return (!(procFlags & PRC_obsolete) && 
				 ((procFlags & PRC_scanned) || noscan) && 
				 !(procFlags & PRC_being_scanned) && 
				 !(procFlags & PRC_being_altered));  };

	//Next
	inline void setNext (Procedure *procedure)
		{procNext = procedure;};
	inline Procedure *findNext ()
		{return procNext;};

	//Format
	inline struct fmt  *findInputFormat ()
		{return procInputFormat;};
		
	inline void setInputFormat (struct fmt *format)
		{procInputFormat = format;};

	inline struct fmt  *findOutputFormat ()
		{return procOutputFormat;};
	inline void setOutputFormat (struct fmt *format)
		{procOutputFormat = format;};

	inline struct fmt *findFormat()
		{return procFormat;};
	
	inline Database *getDatabase()
		{
		return procDatabase;
		}
			
	inline void setFormat (struct fmt *format)
		{
		procFormat = format;
		};

	//Locking
	void lockExistence (tdbb *tdbb);
	void releaseExistence (tdbb *tdbb);
	inline struct lck * findExistenceLock()
		{return procExistenceLock;};
	inline void setExistenceLock (struct lck * existence_lock)
		{procExistenceLock = existence_lock;};
	inline bool hasExistenceLock()
		{return procExistenceLock != NULL;};

	// counts
	inline int findInternalUseCount()
		{return procUseCountInternal;};
	inline void setInternalUseCount (int count)
		{procUseCountInternal = count;};
	inline void incrementInternalUseCount()
		{procUseCountInternal++;};
	inline void decrementInternalUseCount()
		{procUseCountInternal--;};
	inline int findUseCount()
		{return procUseCount;};
	inline void setUseCount(int count)
		{procUseCount = count;};
	inline void incrementUseCount ()
		{procUseCount++;};
	inline void decrementUseCount ()
		{procUseCount--;};
	inline void setAlterCount (int counter)
		{procAlterCount = counter;};
	inline void incremetnAlterCount ()
		{procAlterCount++;};
	inline int findAlterCount ()
		{return procAlterCount;};

	//Request
	inline Request  *findRequest ()
		{return procRequest;};
	
	inline bool hasRequest ()
		{return (procRequest != NULL);};

	//messages
	inline 	struct	jrd_nod	*findInputMsg()
		{return procInputMsg;};
	inline void setInputMsg (struct jrd_nod * msg)
		{procInputMsg = msg;};

	inline 	struct	jrd_nod	*findOutputMsg()
		{return procOutputMsg;};
	inline void setOutputMsg (struct jrd_nod * msg)
		{procOutputMsg = msg;};

	inline const TEXT * findSecurityClassName()
		{return (const TEXT *)procSecurityClassName;};
	inline void setSecurityClassName (const TEXT *name)
		{procSecurityClassName = name;};

	bool operator != (Procedure *proc);
	void setDependencies();

	friend class ProcManager;
	
protected:
	ProcManager		*procManager;
	Procedure		*procNext;
	Database		*procDatabase;
	dsql_sym		*procSymbol;
	ProcParam		*procInputParams;
	ProcParam		*procOutputParams;
	int				procId;
	int				procFlags;
	int				procInputCount;
	int				procOutputCount;
	jrd_nod			*procInputMsg;
	jrd_nod			*procOutputMsg;
	JString			procName;
	JString			procSecurityClassName;
	JString			procOwner;
	fmt				*procInputFormat;
	fmt				*procOutputFormat;
	fmt				*procFormat;
	Request			*procRequest;			/* compiled procedure request */
	int				procUseCount;			/* requests compiled with procedure */
	int				procUseCountInternal;	/* number of procedures compiled with procedure, 
												set and used internally in the MET_clear_cache
												procedure  no code should rely on value of this field 
												(it will usually be 0)
											*/
	lck				*procExistenceLock;		/* existence lock, if any */
	USHORT			procAlterCount;			/* No. of times the procedure was altered */
	GDS_QUAD		procBlobId;
	
public:
	Procedure(Database *dbb, int id);
	void init(void);
	lck* getExistenceLock(tdbb *tdbb);
	static int blockingAst(void* object);
	void blockingAst(void);
	void parseBlr(tdbb *tdbb, const bid *blobId);
	void setRequest(Request* request);
	bool parseMessages(tdbb *tdbb, const UCHAR* blr, int blrLength, Csb* csb);
	void setBlrBlobId(const void* blobId);
};
#endif

