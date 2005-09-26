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
 
// Subsystem.cpp: implementation of the Subsystem class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "Subsystem.h"
#include "iberror.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Subsystem::Subsystem()
{

}

Subsystem::~Subsystem()
{

}
 
ISC_STATUS Subsystem::postError(ISC_STATUS *userStatus, ISC_STATUS code)
{
	/***
	StatusVector statusVector (userStatus);
	statusVector.postCode (code);

	return statusVector.getReturn();
	***/
	ISC_STATUS *p = userStatus;
	*p++ = isc_arg_gds;
	*p++ = code;
	*p++ = isc_arg_end;
	
	return code;
}

int Subsystem::getInterfaceVersion()
{
	return interfaceVersion;
}

ISC_STATUS Subsystem::entrypointUnavailable(ISC_STATUS *userStatus)
{
	return postError (userStatus, isc_unavailable);
}

const TEXT* Subsystem::getDescription()
{
	return "Unknown data manager";
}

ISC_STATUS Subsystem::createDatabase (ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  const UCHAR* dpb,
									  int databaseType, 
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Subsystem::attachDatabase(ISC_STATUS* userStatus, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpb_length, 
									  const UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::databaseInfo(ISC_STATUS* userStatus, DbHandle *dbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::detachDatabase(ISC_STATUS* userStatus, DbHandle *dbHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dropDatabase (ISC_STATUS* userStatus, DbHandle *dbHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Subsystem::startMultiple(ISC_STATUS *userStatus, TraHandle *traHandle, int, const TransactionElement *)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::reconnectTransaction(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int, const UCHAR*)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::transactionInfo(ISC_STATUS* userStatus, TraHandle *traHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::prepareTransaction(ISC_STATUS* userStatus, TraHandle *traHandle, int, const UCHAR*)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::commitRetaining(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::commitTransaction(ISC_STATUS* userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::rollbackRetaining(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::rollbackTransaction(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Subsystem::compileRequest(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::compileRequest2(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::startRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::startAndSend(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::send (ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::requestInfo(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::receive(ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, UCHAR* msg, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::unwindRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::releaseRequest(ISC_STATUS*userStatus, ReqHandle *reqHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Subsystem::createBlob (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::openBlob(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::blobInfo(ISC_STATUS* userStatus, BlbHandle *blbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::putSegment(ISC_STATUS* userStatus, BlbHandle *blbHandle, int segmentLength, UCHAR* segment)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::getSegment (ISC_STATUS* userStatus, BlbHandle *blbHandle, int* segmentLength, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::closeBlob (ISC_STATUS* userStatus, BlbHandle *blbHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::cancelBlob(ISC_STATUS* userStatus, BlbHandle *blbHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Subsystem::putSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR* sdl, int paramLength, UCHAR* param, SLONG sliceLength, UCHAR* slice)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::getSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR *sdl, int paramLength, UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Subsystem::cancelEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::queEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId, int eventsLength, const UCHAR* events, FPTR_VOID ast,void* astArg)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Subsystem::dsqlAllocateStatement(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlAllocateStatement2(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlSqlInfo(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlSetCursor(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, const TEXT* cursorName, int cursorType)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlPrepare(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int sqlLength, const TEXT *sql, int dialect, int itemLength, const UCHAR *items, int bufferLength, UCHAR *buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlDescribe(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlDescribeBind(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, UCHAR* blr, int msgType, int msgLength, const UCHAR* msg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, const UCHAR* blr, int msgType, int msgLength, UCHAR* msg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
								 int sqlLength, const char *sql, int dialect,
								 XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, 
								 int inBlrLength, const UCHAR *inBlr, 
								 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
								 int outBlrLength, const UCHAR *outBlr, 
								 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int dialect, XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	return entrypointUnavailable (userStatus);
}


//ISC_STATUS Subsystem::dsqlExecute2(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int, UCHAR*,int, int, UCHAR*, int, UCHAR*, int, int,UCHAR*)


ISC_STATUS Subsystem::dsqlExecuteImmediate2(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, const UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
										 int outBlrLength, const UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlExecuteImmediate3(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, UCHAR *inMsg, 
										 int outBlrLength, UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, int blrLength, UCHAR *blr, 
										 int msgType, int msgLength, UCHAR* msg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::dsqlFreeStatement(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int option)
{
	return entrypointUnavailable (userStatus);
}



//ISC_STATUS Subsystem::cancelOperation(ISC_STATUS* userStatus, DbHandle *dbHandle, int);
ISC_STATUS Subsystem::serviceQuery(ISC_STATUS *userStatus, 
								SvcHandle *dbHandle, 
								int inItemLength, 
								const UCHAR* inItem, 
								int outItemLength, 
								const UCHAR* outItem, 
								int bufferLength, 
								UCHAR *buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::serviceDetach(ISC_STATUS *userStatus, SvcHandle *dbHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::serviceAttach(ISC_STATUS *userStatus, 
								  const TEXT *service, 
								  SvcHandle *dbHandle, 
								  int spbLength, 
								  const UCHAR *spb, 
								  ConfObject* servicesConfiguration,
								  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::serviceStart(ISC_STATUS* userStatus,
								 SvcHandle *dbHandle,
								 int spbLength, 
								 const UCHAR * spb)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::transactRequest(ISC_STATUS* userStatus, 
								   DbHandle *dbHandle, 
								   TraHandle *traHandle, 
								   int blrLength, 
								   const UCHAR* blr,
								   int inMsgLength, 
								   const UCHAR* inMsg, 
								   int outMsgLength, 
								   UCHAR* outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::executeDDL(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int ddlLength, const UCHAR* ddl)
{
	return entrypointUnavailable (userStatus);
}

/***
int Subsystem::enableSubsystem (TEXT* subSystem)
{
	return false;
}


int Subsystem::disableSubsystem (TEXT* subSystem)
{
	return false;
}
***/

ISC_STATUS Subsystem::databaseCleanup (ISC_STATUS* userStatus, 
									 DbHandle *dbHandle, 
									 DatabaseCleanupRoutine *routine, 
									 SLONG arg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::transactionCleanup (ISC_STATUS* userStatus, 
									   TraHandle *traHandle, 
									   TransactionCleanupRoutine *routine, 
									   SLONG arg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Subsystem::seekBlob (ISC_STATUS* userStatus, 
							 BlbHandle *blbHandle,
							 int mode,
							 SLONG offset,
							 SLONG *result)
{
	return entrypointUnavailable (userStatus);
}




int Subsystem::shutdownConnections(int type, int milliseconds)
{
	return 0;
}

ISC_STATUS Subsystem::cancelOperation(ISC_STATUS* userStatus, DbHandle* dbHandle, int flags)
{
	return postError (userStatus, isc_unavailable);
	//return entrypointUnavailable (userStatus);
}

ISC_STATUS Subsystem::updateAccountInfo(ISC_STATUS* userStatus, DbHandle* dbHandle, int apbLength, const UCHAR* apb)
{
	return entrypointUnavailable (userStatus);
}

ISC_STATUS Subsystem::authenticateUser(ISC_STATUS* userStatus, DbHandle* dbHandle, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}
