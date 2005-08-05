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
 *  Copyright (c) 2003 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */
 
// Engine8.cpp: implementation of the Engine8 class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "firebird.h"
#include "ibase.h"
#include "Engine8.h"
#include "common.h"
#include "jrd_proto.h"
#include "jrd_proto.h"
#include "utl_proto.h"
#include "isc_f_proto.h"
#include "os/isc_i_proto.h"
#include "thd.h"
#include "thd_proto.h"
#include "sch_proto.h"
#include "../dsql/DStatement.h"
#include "PathName.h"
#include "Attachment.h"

static Engine8 provider;
static Subsystem* subsystems [] = { &provider, NULL };

extern "C"
{
Subsystem **getSubsystems()
	{
	return subsystems;
	}
} // extern "C"

//MemMgr	defaultMemoryManager;
	
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Engine8::Engine8()
{

}

Engine8::~Engine8()
{

}

ISC_STATUS Engine8::attachDatabase(ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	THD_init();
	enterSystem();
	
	return exitSystem (jrd8_attach_database (statusVector, orgName, translatedName,
											 (Attachment**) dbHandle, dpbLength, dpb, 
											 databaseConfiguration, providerConfiguration));
}


ISC_STATUS Engine8::blobInfo(ISC_STATUS* statusVector, BlbHandle *blbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();
	
	return exitSystem (jrd8_blob_info (statusVector, (blb**) blbHandle, itemsLength, items, bufferLength, buffer));
}

ISC_STATUS Engine8::cancelBlob(ISC_STATUS *statusVector, BlbHandle *blbHandle)
{
	enterSystem();

	return exitSystem (jrd8_cancel_blob (statusVector, (blb**) blbHandle));
}

ISC_STATUS Engine8::closeBlob(ISC_STATUS *statusVector, BlbHandle *blbHandle)
{
	enterSystem();

	return exitSystem (jrd8_close_blob (statusVector, (blb**) blbHandle));
}

ISC_STATUS Engine8::seekBlob (ISC_STATUS* userStatus, 
							 BlbHandle *blbHandle,
							 int mode,
							 SLONG offset,
							 SLONG *result)
{
	enterSystem();

	return exitSystem (jrd8_seek_blob (userStatus, (blb**) blbHandle, mode, offset, result));
}


ISC_STATUS Engine8::commitTransaction(ISC_STATUS *statusVector, TraHandle *traHandle )
{
	enterSystem();

	return exitSystem (jrd8_commit_transaction (statusVector, (Transaction**) traHandle));
}

ISC_STATUS Engine8::compileRequest(ISC_STATUS *statusVector, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR *blr)
{
	enterSystem();

	return exitSystem (jrd8_compile_request (statusVector, 
								 (Attachment**) dbHandle, 
								 (Request**) reqHandle, blrLength, blr));
}

ISC_STATUS Engine8::createBlob (ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	enterSystem();

	return exitSystem (jrd8_create_blob2 (statusVector, 
										(Attachment**) dbHandle,
										(Transaction**) traHandle,
										(blb**) blbHandle, 
										blobId,
										bpbLength,
										bpb));
}

/***
ISC_STATUS Engine8::createDatabase (ISC_STATUS* statusVector, 
									int fileLength, 
									const TEXT* fileName, D
									DbHandle *dbHandle, 
									int dpbLength, 
									UCHAR* dpb, 
									int databaseType, 
									const char *providerName)
***/
ISC_STATUS Engine8::createDatabase (ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  UCHAR* dpb,
									  int databaseType, 
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	THD_init();
	enterSystem();

	return exitSystem (jrd8_create_database (statusVector, 
							orgName,
							translatedName,
							(Attachment**) dbHandle, 
							dpbLength, dpb, 
							databaseType, 
							databaseConfiguration, providerConfiguration));
}

ISC_STATUS Engine8::databaseInfo(ISC_STATUS *statusVector, DbHandle *dbHandle, int itemLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();

	return exitSystem (jrd8_database_info (statusVector, 
								 (Attachment**) dbHandle, 
								 itemLength, (const UCHAR*) items, 
								 bufferLength, (UCHAR*) buffer));
}

ISC_STATUS Engine8::detachDatabase(ISC_STATUS *statusVector, DbHandle *dbHandle )
{
	enterSystem();

	return exitSystem (jrd8_detach_database (statusVector, (Attachment**) dbHandle));
}

ISC_STATUS Engine8::getSegment(ISC_STATUS *statusVector, BlbHandle *blbHandle, int *segmentLength, int bufferLength, UCHAR *buffer)
{
	enterSystem();

	return exitSystem (jrd8_get_segment (statusVector, (blb**) blbHandle, segmentLength, bufferLength, buffer));
}

ISC_STATUS Engine8::openBlob(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	enterSystem();

	return exitSystem (jrd8_open_blob2 (statusVector, 
										(Attachment**) dbHandle,
										(Transaction**) traHandle,
										(blb**) blbHandle, 
										blobId,
										bpbLength,
										bpb));
}

ISC_STATUS Engine8::prepareTransaction(ISC_STATUS *statusVector, TraHandle *traHandle, int msgLength, UCHAR* msg)
{
	enterSystem();

	return exitSystem (jrd8_prepare_transaction (statusVector, (Transaction**) traHandle, msgLength, msg));
}

ISC_STATUS Engine8::reconnectTransaction(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, int, UCHAR *)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::receive(ISC_STATUS *statusVector, ReqHandle *reqHandle, int msgType, int msgLength, UCHAR *msg, int level)
{
	enterSystem();

	return exitSystem (jrd8_receive (statusVector, 
								 (Request**) reqHandle, msgType, msgLength, msg, level));
}

ISC_STATUS Engine8::requestInfo(ISC_STATUS* statusVector, ReqHandle *reqHandle, int level, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return exitSystem (jrd8_request_info (statusVector, 
								 (Request**) reqHandle, level,
								 itemsLength, (const UCHAR*) items, 
								 bufferLength, (UCHAR*) buffer));
}

ISC_STATUS Engine8::rollbackTransaction(ISC_STATUS *statusVector, TraHandle *traHandle )
{
	enterSystem();

	return exitSystem (jrd8_rollback_transaction (statusVector, (Transaction**) traHandle));
}

ISC_STATUS Engine8::send (ISC_STATUS* statusVector, ReqHandle *reqHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	enterSystem();

	return exitSystem (jrd8_send (statusVector, 
								 (Request**) reqHandle, msgType, msgLength, msg, level));
}

ISC_STATUS Engine8::startRequest(ISC_STATUS *statusVector, ReqHandle *reqHandle, TraHandle *traHandle, int level)
{
	enterSystem();

	return exitSystem (jrd8_start_request (statusVector, 
					   (Request**) reqHandle,
					   (Transaction**) traHandle, 
					   level));
}

ISC_STATUS Engine8::startAndSend(ISC_STATUS *statusVector, ReqHandle *reqHandle, TraHandle *traHandle, int msgType, int msgLength, const UCHAR *msg, int level)
{
	enterSystem();

	return exitSystem (jrd8_start_and_send (statusVector, 
					   (Request**) reqHandle,
					   (Transaction**) traHandle, 
					   msgType,
					   msgLength,
					   msg,
					   level));
}

ISC_STATUS Engine8::startMultiple(ISC_STATUS *statusVector, TraHandle *traHandle, int count, teb *tebs)
{
	enterSystem();

	return exitSystem (jrd8_start_multiple (statusVector, (Transaction**) traHandle, count, tebs));
}

ISC_STATUS Engine8::transactionInfo(ISC_STATUS* statusVector, TraHandle *traHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::unwindRequest(ISC_STATUS *statusVector, ReqHandle *reqHandle, int level)
{
	return exitSystem (jrd8_unwind_request(statusVector, (Request **)reqHandle, level));
}

ISC_STATUS Engine8::commitRetaining(ISC_STATUS *statusVector, TraHandle *traHandle )
{
	enterSystem();

	return exitSystem (jrd8_commit_retaining (statusVector, (Transaction**) traHandle));
}

ISC_STATUS Engine8::queEvents(ISC_STATUS *statusVector, DbHandle *dbHandle, SLONG *, int, UCHAR *, FPTR_VOID, void *)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::cancelEvents(ISC_STATUS *statusVector, DbHandle *dbHandle, SLONG *)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::executeDDL(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, int ddlLength, UCHAR *ddl)
{
	enterSystem();

	return exitSystem (jrd8_ddl (statusVector, 
								 (Attachment**) dbHandle,
								 (Transaction**) traHandle,
								 ddlLength, ddl));
}

ISC_STATUS Engine8::getSlice(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
							 SLONG *arrayId, int sdlLength, UCHAR *sdl, int paramLength, 
							 UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
{
	enterSystem();

	return exitSystem (jrd8_get_slice(statusVector, (Attachment**) dbHandle, (Transaction**) traHandle, 
					   arrayId, sdlLength, sdl, paramLength, param, sliceLength, slice, returnLength));
}

ISC_STATUS Engine8::putSlice(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
							 SLONG *arrayId, int sdlLength, UCHAR *sdl, int paramLength, 
							 UCHAR *param, SLONG sliceLength, UCHAR *slice)
{
	enterSystem();

	return exitSystem (jrd8_put_slice(statusVector, (Attachment**) dbHandle, (Transaction**) traHandle, 
					   arrayId, sdlLength, sdl, paramLength, param, sliceLength, slice));
}

ISC_STATUS Engine8::transactRequest(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, int, UCHAR *, int, UCHAR *, int, UCHAR *)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::dropDatabase(ISC_STATUS *statusVector, DbHandle *dbHandle )
{
	enterSystem();

	return exitSystem (jrd8_drop_database (statusVector, (Attachment**) dbHandle));
}

ISC_STATUS Engine8::dsqlAllocateStatement(ISC_STATUS *statusVector, DbHandle *dbHandle, DsqlHandle *dsqlHandle )
{
	enterSystem();

	DStatement *statement = ((Attachment*) *dbHandle)->allocateStatement();
	*dsqlHandle = (DsqlHandle) statement;

	return exitSystem (0);
}

ISC_STATUS Engine8::dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, 
								 int inBlrLength, UCHAR *inBlr, 
								 int inMsgType, int inMsgLength, UCHAR *inMsg, 
								 int outBlrLength, UCHAR *outBlr, 
								 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	enterSystem();
	//Transaction *transaction = (Transaction*) *traHandle;
	DStatement *statement = (DStatement*) *dsqlHandle;

	return exitSystem (statement->execute (userStatus, 
											(Transaction**) traHandle,
											inBlrLength, inBlr, 
											inMsgType, inMsgLength, inMsg, 
											outBlrLength, outBlr, 
											outMsgType, outMsgLength, outMsg));
}

ISC_STATUS Engine8::dsqlExecute2(ISC_STATUS *statusVector, TraHandle *traHandle, DsqlHandle *dsqlHandle, int, UCHAR *, int, int, UCHAR *, int, UCHAR *, int, int, UCHAR *)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::dsqlExecuteImmediate(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
										int sqlLength, const char* sql, int dialect, int blrLength, UCHAR *blr, 
										int msgType, int msgLength, UCHAR* msg)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::dsqlExecuteImmediate2(ISC_STATUS *statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
											 int sqlLength, const char* sql, int dialect, 
											 int inBlrLength, UCHAR *inBlr, 
											 int inMsgType, int inMsgLength, UCHAR *inMsg, 
											 int outBlrLength, UCHAR *outBlr, 
											 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	enterSystem();
	DStatement *statement = ((Attachment*) *dbHandle)->allocateStatement();
	ISC_STATUS ret = statement->executeImmediate (statusVector,
									(Transaction**) traHandle,
									sqlLength, sql, dialect,
									inBlrLength, inBlr,
									inMsgType, inMsgLength, inMsg,
									outBlrLength, outBlr,
									outMsgType, outMsgLength, outMsg);
	delete statement;	
	
	return exitSystem (ret);
}

ISC_STATUS Engine8::dsqlFetch(ISC_STATUS *statusVector, DsqlHandle *dsqlHandle, 
							  int blrLength, const UCHAR* blr, int msgType, 
							  int msgLength, UCHAR* msg)
{
	enterSystem();
	DStatement *statement = (DStatement*) *dsqlHandle;
	
	return exitSystem (statement->fetch (statusVector, blrLength, blr, msgType, msgLength, msg));
}

ISC_STATUS Engine8::dsqlFreeStatement(ISC_STATUS *statusVector, DsqlHandle *dsqlHandle, int options)
{
	enterSystem();
	DStatement *statement = (DStatement*) *dsqlHandle;
	
	ISC_STATUS ret = statement->freeStatement (statusVector, options);
	
	if (!ret && (options & DSQL_drop))
		*dsqlHandle = NULL;

	return exitSystem (ret);
}

ISC_STATUS Engine8::dsqlInsert(ISC_STATUS *statusVector, DsqlHandle *dsqlHandle, int blrLength, UCHAR* blr, int msgType, int msgLength, const UCHAR* msg)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Engine8::dsqlPrepare(ISC_STATUS *statusVector, 
								TraHandle *traHandle, 
								DsqlHandle *dsqlHandle, 
								int sqlLength, 
								const TEXT *sql, 
								int dialect, 
								int itemLength, 
								const UCHAR *items, 
								int bufferLength, 
								UCHAR *buffer)
{
	enterSystem();
	Transaction *transaction = (Transaction*) *traHandle;
	DStatement *statement = (DStatement*) *dsqlHandle;
	return exitSystem (statement->prepare (statusVector, transaction, sqlLength, sql, dialect, itemLength, items, bufferLength, buffer));
}

ISC_STATUS Engine8::dsqlSetCursor(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, const TEXT* cursorName, int cursorType)
{
	enterSystem();
	DStatement *statement = (DStatement*) *dsqlHandle;
	
	return exitSystem (statement->setCursor (statusVector, cursorName, cursorType));
}

ISC_STATUS Engine8::dsqlSqlInfo(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();
	DStatement *statement = (DStatement*) *dsqlHandle;
	
	return exitSystem (statement->getSqlInfo (statusVector, itemsLength, items, bufferLength, buffer));
}

ISC_STATUS Engine8::serviceAttach(ISC_STATUS *statusVector, const TEXT *, SvcHandle *dbHandle, int, UCHAR*,
								  ConfObject* servicesConfiguration,
								  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::serviceDetach(ISC_STATUS *statusVector, SvcHandle *dbHandle )
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::serviceQuery(ISC_STATUS *statusVector, SvcHandle *dbHandle, int, UCHAR *, int, UCHAR *, int, UCHAR *)
{
	return entrypointUnavailable (statusVector);
}

ISC_STATUS Engine8::rollbackRetaining(ISC_STATUS *statusVector, TraHandle *traHandle )
{
	enterSystem();

	return exitSystem (jrd8_rollback_retaining (statusVector, (Transaction**) traHandle));
}

ISC_STATUS Engine8::cancelOperation(ISC_STATUS *statusVector, DbHandle *dbHandle, int op)
{
	enterSystem();
	
	return exitSystem (jrd8_cancel_operation (statusVector, (Attachment**) dbHandle, op));
}

ISC_STATUS Engine8::releaseRequest(ISC_STATUS *statusVector, ReqHandle *reqHandle )
{
	enterSystem();

	return exitSystem (jrd8_release_request (statusVector, (Request**) reqHandle));
}

ISC_STATUS Engine8::putSegment(ISC_STATUS *statusVector, BlbHandle *blbHandle, int segmentLength, UCHAR *segment)
{
	enterSystem();

	return exitSystem (jrd8_put_segment (statusVector, (blb**) blbHandle, segmentLength, segment));
}

const TEXT* Engine8::getDescription()
{
	return "Firebird/Vulcan ODS 8 Data Manager";
}

void Engine8::enterSystem()
{
	THREAD_ENTER;
	ISC_enter();
}

ISC_STATUS Engine8::exitSystem(ISC_STATUS code)
{
	ISC_exit();
	THREAD_EXIT;

	return code;
}

ISC_STATUS Engine8::updateAccountInfo(ISC_STATUS* statusVector, DbHandle* dbHandle, int apbLength, const UCHAR* apb)
{
	enterSystem();
	
	return exitSystem (jrd8_update_account_info (statusVector, (Attachment**) dbHandle,
												 apbLength, apb));
}

ISC_STATUS Engine8::authenticateUser(ISC_STATUS* statusVector, DbHandle* dbHandle, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();
	
	return exitSystem (jrd8_user_info (statusVector,(Attachment**) dbHandle,
									   dpbLength, dpb,
									   itemsLength, items,
									   bufferLength, buffer));
}
