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

// SubsysHandle.h: interface for the SubsysHandle class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SUBSYSHANDLE_H__0201B19B_85DD_42FF_8547_2612212326A0__INCLUDED_)
#define AFX_SUBSYSHANDLE_H__0201B19B_85DD_42FF_8547_2612212326A0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Subsystem.h"

class YRequest;
class YStatement;
class YTransaction;
class YBlob;

class SubsysHandle  
{
public:
	void removeStatement (YStatement *statement);
	void addStatement (YStatement *statement);
	void releaseStatements();
	void transactionEnded (YTransaction *transaction);
	void transactionStarted (YTransaction *transaction);
	void releaseRequests();
	void removeRequest  (YRequest *request);
	void addRequest (YRequest *request);
	SubsysHandle(Subsystem *sys, DbHandle orgHandle);
	virtual ~SubsysHandle();

	DbHandle	handle;
	Subsystem	*subsystem;
	YRequest	*firstRequest;
	YRequest	*lastRequest;
	YStatement	*firstStatement;
	YStatement	*lastStatement;
	YBlob		*firstBlob;
	YBlob		*lastBlob;
	int			activeTransactions;
protected:
public:
	void addBlob(YBlob* blob);
	void removeBlob(YBlob* blob);
	void cancelBlobs(void);
};

#endif // !defined(AFX_SUBSYSHANDLE_H__0201B19B_85DD_42FF_8547_2612212326A0__INCLUDED_)
