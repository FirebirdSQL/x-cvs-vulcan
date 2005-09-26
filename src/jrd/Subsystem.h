/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by [Initial Developer's Name]
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2003 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */
 
 // Subsystem.h: interface for the Subsystem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SUBSYSTEM_H__AE519A29_124C_411B_BF19_9BB7F234FC77__INCLUDED_)
#define AFX_SUBSYSTEM_H__AE519A29_124C_411B_BF19_9BB7F234FC77__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "firebird.h"
#include "../dsql/sqlda.h"

static const int interfaceVersion1	= 1;
static const int interfaceVersion	= interfaceVersion1;

typedef void*	DbHandle;
typedef void*	TraHandle;
typedef void*	ReqHandle;
typedef void*	BlbHandle;
typedef void*	DsqlHandle;
typedef void*	SvcHandle;

struct TransactionElement {
	DbHandle	*dbHandle;
	int			tpbLength;
	const UCHAR	*tpb;
	};

typedef void DatabaseCleanupRoutine(DbHandle*, SLONG);
typedef void TransactionCleanupRoutine(TraHandle*, SLONG);

CLASS(ConfObject);

struct bid {
	int relationId;
	int blobNumber;
	};
	
//struct XSQLDA;

class Subsystem  
{
public:
	virtual ISC_STATUS createDatabase(ISC_STATUS* userStatus, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpb_length, 
									  const UCHAR* dpb,
									  int databaseType, 
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration); 
	virtual ISC_STATUS attachDatabase(ISC_STATUS* userStatus, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpb_length, 
									  const UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration); 
	virtual ISC_STATUS databaseInfo(ISC_STATUS* userStatus, DbHandle *dbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS detachDatabase(ISC_STATUS* userStatus, DbHandle *dbHandle);
	virtual ISC_STATUS dropDatabase (ISC_STATUS* userStatus, DbHandle *dbHandle);

	virtual ISC_STATUS startMultiple(ISC_STATUS *, TraHandle *traHandle, int, const TransactionElement *);
	virtual ISC_STATUS reconnectTransaction(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int, const UCHAR*);
	virtual ISC_STATUS transactionInfo(ISC_STATUS* userStatus, TraHandle *traHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS prepareTransaction(ISC_STATUS* userStatus, TraHandle *traHandle, int msgLength, const UCHAR* msg);
	virtual ISC_STATUS commitRetaining(ISC_STATUS *, TraHandle *traHandle);
	virtual ISC_STATUS commitTransaction(ISC_STATUS* userStatus, TraHandle *traHandle);
	virtual ISC_STATUS rollbackRetaining(ISC_STATUS *, TraHandle *traHandle);
	virtual ISC_STATUS rollbackTransaction(ISC_STATUS *, TraHandle *traHandle);

	virtual ISC_STATUS compileRequest(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr);
	virtual ISC_STATUS compileRequest2(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr);
	virtual ISC_STATUS startRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int level);
	virtual ISC_STATUS startAndSend(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int msgType, int msgLength, const UCHAR* msg, int level);
	virtual ISC_STATUS send (ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, const UCHAR* msg, int level);
	virtual ISC_STATUS requestInfo(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS receive(ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, UCHAR* msg, int level);
	virtual ISC_STATUS unwindRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level);
	virtual ISC_STATUS releaseRequest(ISC_STATUS*statusVector, ReqHandle *reqHandle);

	virtual ISC_STATUS createBlob (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb);
	virtual ISC_STATUS openBlob(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb);
	virtual ISC_STATUS blobInfo(ISC_STATUS* userStatus, BlbHandle *blbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS putSegment(ISC_STATUS* userStatus, BlbHandle *blbHandle, int segmentLength, UCHAR* segment);
	virtual ISC_STATUS getSegment (ISC_STATUS* userStatus, BlbHandle *blbHandle, int* segmentLength, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS closeBlob (ISC_STATUS* userStatus, BlbHandle *blbHandle);
	virtual ISC_STATUS cancelBlob(ISC_STATUS* userStatus, BlbHandle *blbHandle);

	virtual ISC_STATUS putSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR* sdl, int paramLength, UCHAR* param, SLONG sliceLength, UCHAR* slice);
	virtual ISC_STATUS getSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR *sdl, int paramLength, UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength);

	virtual ISC_STATUS cancelEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId);
	virtual ISC_STATUS queEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId, int eventsLength, const UCHAR* events, FPTR_VOID ast,void* astArg);

	virtual ISC_STATUS dsqlAllocateStatement(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle);
	virtual ISC_STATUS dsqlAllocateStatement2(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle);
	virtual ISC_STATUS dsqlSqlInfo(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS dsqlSetCursor(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, const TEXT* cursorName, int cursorType);
	virtual ISC_STATUS dsqlPrepare(ISC_STATUS* userStatus, 
								   TraHandle *traHandle, 
								   DsqlHandle *dsqlHandle, 
								   int sqlLength, 
								   const TEXT *sql, 
								   int dialect, 
								   int itemLength, 
								   const UCHAR *items, 
								   int bufferLength, 
								   UCHAR *buffer);
	virtual ISC_STATUS dsqlDescribe(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda);
	virtual ISC_STATUS dsqlDescribeBind(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda);
	virtual ISC_STATUS dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda);
	virtual ISC_STATUS dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, UCHAR* blr, int msgType, int msgLength, const UCHAR* msg);
	virtual ISC_STATUS dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda);
	virtual ISC_STATUS dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, const UCHAR* blr, int msgType, int msgLength, UCHAR* msg);
	virtual ISC_STATUS dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
									 int sqlLength, const char *sql, int dialect,
									 XSQLDA *inSqlda, XSQLDA *outSqlda);
	virtual ISC_STATUS dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle,
									 int inBlrLength, const UCHAR *inBlr, 
									 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
									 int outBlrLength, const UCHAR *outBlr, 
									 int outMsgType, int outMsgLength, UCHAR *outMsg);
	virtual ISC_STATUS dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int dialect, XSQLDA *inSqlda, XSQLDA *outSqlda);
	//virtual ISC_STATUS dsqlExecute2(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int, UCHAR*,int, int, UCHAR*, int, UCHAR*, int, int,UCHAR*);
	virtual ISC_STATUS dsqlExecuteImmediate2(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
											 int sqlLength, const char* sql, int dialect, 
											 int inBlrLength, const UCHAR *inBlr, 
											 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
											 int outBlrLength, const UCHAR *outBlr, 
											 int outMsgType, int outMsgLength, UCHAR *outMsg);
	virtual ISC_STATUS dsqlExecuteImmediate3(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
											 int sqlLength, const char* sql, int dialect, 
											 int inBlrLength, UCHAR *inBlr, 
											 int inMsgType, int inMsgLength, UCHAR *inMsg, 
											 int outBlrLength, UCHAR *outBlr, 
											 int outMsgType, int outMsgLength, UCHAR *outMsg);
	virtual ISC_STATUS dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
											 int sqlLength, const char* sql, int dialect, int blrLength, UCHAR *blr, 
											 int msgType, int msgLength, UCHAR* msg);
	virtual ISC_STATUS dsqlFreeStatement(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int option);

	//virtual ISC_STATUS cancelOperation(ISC_STATUS* userStatus, DbHandle *dbHandle, int);
	virtual ISC_STATUS serviceQuery(ISC_STATUS *userStatus, 
									SvcHandle *dbHandle, 
									int inItemLength, 
									const UCHAR* inItem, 
									int outItemLength, 
									const UCHAR* outItem, 
									int bufferLength, 
									UCHAR *buffer);
	virtual ISC_STATUS serviceDetach(ISC_STATUS *userStatus, SvcHandle *dbHandle);
	virtual ISC_STATUS serviceAttach(ISC_STATUS *userStatus, 
									  const TEXT *service, 
									  SvcHandle *dbHandle, 
									  int spbLength, 
									  const UCHAR *spb,
								      ConfObject* servicesConfiguration,
								      ConfObject* providerConfiguration);
	virtual ISC_STATUS serviceStart(ISC_STATUS* userStatus,
									 SvcHandle *dbHandle,
									 int spbLength, 
									 const UCHAR * spb);
	virtual ISC_STATUS transactRequest(ISC_STATUS* userStatus, 
									   DbHandle *dbHandle, 
									   TraHandle *traHandle, 
									   int blrLength, 
									   const UCHAR* blr,
									   int inMsgLength, 
									   const UCHAR* inMsg, 
									   int outMsgLength, 
									   UCHAR* outMsg);
	virtual ISC_STATUS executeDDL(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int ddlLength, const UCHAR* ddl);
	//virtual int enableSubsystem (TEXT* subSystem);
	//virtual int disableSubsystem (TEXT* subSystem);
	virtual ISC_STATUS databaseCleanup (ISC_STATUS* userStatus, 
										 DbHandle *dbHandle, 
										 DatabaseCleanupRoutine *routine, 
										 SLONG arg);
	virtual ISC_STATUS transactionCleanup (ISC_STATUS* userStatus, 
										   TraHandle *traHandle, 
										   TransactionCleanupRoutine *routine, 
										   SLONG arg);
	virtual ISC_STATUS seekBlob (ISC_STATUS* userStatus, 
								 BlbHandle *blbHandle,
								 int mode,
								 SLONG offset,
								 SLONG *result);

	Subsystem();
	virtual ~Subsystem();

	int getInterfaceVersion();
	virtual const TEXT* getDescription();
	virtual ISC_STATUS entrypointUnavailable(ISC_STATUS *statusVector);
	virtual ISC_STATUS postError (ISC_STATUS *statusVector, ISC_STATUS code);
	virtual int shutdownConnections(int type, int milliseconds);
	virtual ISC_STATUS cancelOperation(ISC_STATUS* userStatus, DbHandle* dbHandle, int flags);
	virtual ISC_STATUS updateAccountInfo(ISC_STATUS* userStatus, DbHandle* dbHandle, int apbLength, const UCHAR* apb);
	virtual ISC_STATUS authenticateUser(ISC_STATUS* userStatus, DbHandle* dbHandle, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
};

#endif // !defined(AFX_SUBSYSTEM_H__AE519A29_124C_411B_BF19_9BB7F234FC77__INCLUDED_)
