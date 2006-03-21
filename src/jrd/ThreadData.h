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

// ThreadData.h: interface for the ThreadData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREADDATA_H__1691120B_01BB_4718_86F5_F0A5A2FEDBF9__INCLUDED_)
#define AFX_THREADDATA_H__1691120B_01BB_4718_86F5_F0A5A2FEDBF9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tdbb.h"

class Database;
class Attachment;
struct thread_db;
class JrdMemoryPool;

class ThreadData  
{
public:
	ISC_STATUS getStatus();
	void setDsqlPool (DsqlMemoryPool *pool);
	JrdMemoryPool* getPool();
	ISC_STATUS* getStatusVector();
	Attachment* getAttachment();
	Database* getDatabase();
	void setPool (JrdMemoryPool *pool);
	void setStatusVector (ISC_STATUS *statusVector);
	void setAttachment (Attachment *attachment);
	ThreadData (ISC_STATUS *statusVector);
	void setDatabase (Database *database);
	ThreadData (ISC_STATUS *statusVector, Database *database);
	void init (ISC_STATUS *statusVector, Database *database);
	ThreadData(ISC_STATUS *statusVector, Attachment *attachment);
	virtual ~ThreadData();
	operator thread_db*();

	thread_db	thd_context;
	thread_db*	threadData;
	void setTransaction(Transaction* transaction);
};

#endif // !defined(AFX_THREADDATA_H__1691120B_01BB_4718_86F5_F0A5A2FEDBF9__INCLUDED_)
