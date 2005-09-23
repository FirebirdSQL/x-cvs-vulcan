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

// YTransaction.h: interface for the YTransaction class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_YTRANSACTION_H__8FF55D15_110C_4BB4_A7E4_3F529B5AC96E__INCLUDED_)
#define AFX_YTRANSACTION_H__8FF55D15_110C_4BB4_A7E4_3F529B5AC96E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Dispatch.h"

#define LOCAL_VECTOR_SIZE	5

class StatusVector;
class SubsysHandle;
class HandleManager;

//struct teb;

struct TranDb
	{
	SubsysHandle		*subsystem;
	TransactionElement	element;
	TraHandle			handle;
	};

class YTransaction  
{
public:
	ISC_STATUS commit(StatusVector &statusVector);
	DbHandle getDbHandle (SubsysHandle *subsystem);
	ISC_STATUS rollback (StatusVector& statusVector);
	ISC_STATUS start(StatusVector &statusVector);
	YTransaction(int dbCount);
	virtual ~YTransaction();

protected:
	int			numberDatabases;
	int			numberActive;
	bool		inLimbo;
	TranDb		*databases;	
	TranDb		localDatabases [LOCAL_VECTOR_SIZE];
public:
	ISC_STATUS commitRetaining(StatusVector& statusVector);
	ISC_STATUS rollbackRetaining(StatusVector& statusVector);
	void setDatabase(int index, SubsysHandle* handle, int tdbLength, UCHAR* tpb);
	void setTransactionHandle(SubsysHandle* subsystem, TraHandle transactionHandle);
	ISC_STATUS prepare(StatusVector& statusVector, int msgLength, const UCHAR* msg);
};

#endif // !defined(AFX_YTRANSACTION_H__8FF55D15_110C_4BB4_A7E4_3F529B5AC96E__INCLUDED_)
