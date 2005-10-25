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

// BlrGen.h: interface for the BlrGen class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _INTERNAL_SECURITY_CONTEXT_H
#define _INTERNAL_SECURITY_CONTEXT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SecurityPlugin.h"
#include "ibase.h"

class InternalConnection;
class Transaction;
class thread_db;
class Attachment;

class InternalSecurityContext : public SecurityContext
{
public:
	InternalSecurityContext(thread_db* tdbb, Attachment *attachment);
	virtual ~InternalSecurityContext(void);
	virtual Connection*		getUserConnection(void);
	virtual Connection*		getNewConnection(void);
	virtual void			commit(void);
	virtual void			rollback(void);
	virtual const char*		getDatabaseFilename(void);
	
	Transaction			*transaction;
	Attachment			*attachment;
	InternalConnection	*connection;
	thread_db			*threadData;
	isc_db_handle		dbHandle;
	bool				internalTransaction;
	
	virtual const char* getAccount(void);
	virtual const char* getEncryptedPassword(void);
	virtual const char* getPassword(void);
};

#endif

