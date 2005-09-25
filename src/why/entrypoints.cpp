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

#include <stdarg.h>
#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "Dispatch.h"
#include "iberror.h"
#include "Mutex.h"

//static Dispatch dispatch (traceCalls);
//extern Dispatch dispatch;
static Dispatch	*dispatch;
static Mutex	mutex;

static void initialize()
{
	mutex.lock();
	
	if (!dispatch)
		dispatch = new Dispatch(0);
	
	mutex.release();
}

JString truncateName(int fileLength, const TEXT *fileName)
{
	const char *p = fileName + fileLength;
	
	while (p > fileName && p[-1] == ' ')
		--p;
		
	return JString(fileName, fileName - p);
}

extern "C" {

ISC_STATUS API_ROUTINE isc_create_database (ISC_STATUS* userStatus, 
									   USHORT fileLength, 
									   const TEXT* fileName, 
									   DbHandle *dbHandle, 
									   USHORT dpbLength, 
									   UCHAR* dpb, 
									   USHORT databaseType)
	{
	if (!dispatch)
		initialize();
		
	JString temp;
	const char *name = fileName;
	
	if (fileLength)
		name = temp = truncateName(fileLength, fileName);
		
	if (!dispatch)
		initialize();
		
	return dispatch->createDatabase (userStatus, name, name, dbHandle, dpbLength, dpb, databaseType, NULL, NULL);
	}

ISC_STATUS API_ROUTINE isc_attach_database(ISC_STATUS* userStatus, 
										   SSHORT fileLength, 
										   const TEXT* fileName, 
										   DbHandle *dbHandle, 
										   SSHORT dpbLength, 
										   UCHAR* dpb)
	{
	if (!dispatch)
		initialize();
		
	JString temp;
	const char *name = fileName;
	
	if (fileLength)
		name = temp = truncateName(fileLength, fileName);
		
	return dispatch->attachDatabase (userStatus, name, name, dbHandle, dpbLength, dpb, NULL, NULL);
	}


ISC_STATUS API_ROUTINE isc_database_info(ISC_STATUS* userStatus, DbHandle *dbHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->databaseInfo (userStatus, dbHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE isc_detach_database(ISC_STATUS* userStatus, DbHandle *dbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->detachDatabase (userStatus, dbHandle);
	}


ISC_STATUS API_ROUTINE isc_drop_database (ISC_STATUS* userStatus, DbHandle *dbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dropDatabase (userStatus, dbHandle);
	}


ISC_STATUS API_ROUTINE isc_start_multiple(ISC_STATUS* userStatus, TraHandle *traHandle, USHORT count, const TransactionElement *stuff)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->startMultiple (userStatus, traHandle, count, stuff);
	}


ISC_STATUS API_ROUTINE_VARARG isc_start_transaction (ISC_STATUS* userStatus, TraHandle *traHandle, SSHORT count, ...)
	{
	TransactionElement vector [32];
	va_list ptr;
	VA_START(ptr, count);

	for (TransactionElement *t = vector, *end = vector + count; t < end; ++t)
		{
		t->dbHandle = va_arg(ptr, DbHandle*);
		t->tpbLength = va_arg(ptr, int);
		t->tpb = va_arg(ptr, UCHAR *);
		}

	if (!dispatch)
		initialize();
		
	return isc_start_multiple (userStatus, traHandle, count, vector);
	}

ISC_STATUS API_ROUTINE isc_reconnect_transaction(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SSHORT length, UCHAR* id)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->reconnectTransaction (userStatus, dbHandle, traHandle, length, id);
	}


ISC_STATUS API_ROUTINE isc_transaction_info(ISC_STATUS* userStatus, TraHandle *traHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->transactionInfo (userStatus, traHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE isc_prepare_transaction(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->prepareTransaction (userStatus, traHandle, 0, 0);
	}


ISC_STATUS API_ROUTINE isc_prepare_transaction2 (ISC_STATUS* userStatus, TraHandle *traHandle, USHORT msgLength, UCHAR* msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->prepareTransaction (userStatus, traHandle, msgLength, msg);
	}


ISC_STATUS API_ROUTINE isc_commit_retaining(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->commitRetaining (userStatus, traHandle);
	}


ISC_STATUS API_ROUTINE isc_commit_transaction(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->commitTransaction (userStatus, traHandle);
	}


ISC_STATUS API_ROUTINE isc_rollback_retaining(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->rollbackRetaining (userStatus, traHandle);
	}


ISC_STATUS API_ROUTINE isc_rollback_transaction(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->rollbackTransaction (userStatus, traHandle);
	}



ISC_STATUS API_ROUTINE isc_compile_request(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, SSHORT blrLength, UCHAR* blr)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->compileRequest (userStatus, dbHandle, reqHandle, blrLength, blr);
	}

ISC_STATUS API_ROUTINE isc_compile_request2(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, SSHORT blrLength, UCHAR* blr)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->compileRequest2 (userStatus, dbHandle, reqHandle, blrLength, blr);
	}


ISC_STATUS API_ROUTINE isc_start_request (ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->startRequest (userStatus, reqHandle, traHandle, level);
	}

ISC_STATUS API_ROUTINE isc_start_and_send(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle,USHORT msgType, USHORT msgLength, UCHAR *msg, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->startAndSend (userStatus, reqHandle, traHandle, msgType, msgLength, msg, level);
	}


ISC_STATUS API_ROUTINE isc_send (ISC_STATUS* userStatus, ReqHandle *reqHandle, USHORT msgType, USHORT msgLength, UCHAR *msg, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->send (userStatus, reqHandle, msgType, msgLength, msg, level);
	}


ISC_STATUS API_ROUTINE isc_request_info(ISC_STATUS* userStatus, ReqHandle *reqHandle, SSHORT level, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->requestInfo (userStatus, reqHandle, level, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE isc_receive(ISC_STATUS* userStatus, ReqHandle *reqHandle, USHORT msgType, USHORT msgLength, UCHAR *msg, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->receive (userStatus, reqHandle, msgType, msgLength, msg, level);
	}


ISC_STATUS API_ROUTINE isc_unwind_request(ISC_STATUS* userStatus, ReqHandle *reqHandle, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->unwindRequest (userStatus, reqHandle, level);
	}


ISC_STATUS API_ROUTINE isc_release_request(ISC_STATUS* userStatus, ReqHandle *reqHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->releaseRequest (userStatus, reqHandle);
	}



ISC_STATUS API_ROUTINE isc_create_blob (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid * blobId)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->createBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, 0, NULL);
	}


ISC_STATUS API_ROUTINE isc_create_blob2 (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid * blobId, USHORT bpbLength, UCHAR* bpb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->createBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, bpbLength, bpb);
	}


ISC_STATUS API_ROUTINE isc_open_blob(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->openBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, 0, 0);
	}


ISC_STATUS API_ROUTINE isc_open_blob2(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, USHORT bpbLength, UCHAR* bpb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->openBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, bpbLength, bpb);
	}


ISC_STATUS API_ROUTINE isc_blob_info(ISC_STATUS* userStatus, BlbHandle *blbHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->blobInfo (userStatus, blbHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE isc_put_segment(ISC_STATUS* userStatus, BlbHandle *blbHandle, USHORT segmentLength, UCHAR* segment)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->putSegment (userStatus, blbHandle, segmentLength, segment);
	}


ISC_STATUS API_ROUTINE isc_get_segment (ISC_STATUS* userStatus, BlbHandle *blbHandle, USHORT* segmentLength, USHORT bufferLength, UCHAR* buffer)
	{
	int length;
	ISC_STATUS ret = dispatch->getSegment (userStatus, blbHandle, &length, bufferLength, buffer);
	
	if (!ret || ret == isc_segstr_eof || ret == isc_segment)
		*segmentLength = length;
	
	return ret;
	}


ISC_STATUS API_ROUTINE isc_close_blob (ISC_STATUS* userStatus, BlbHandle *blbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->closeBlob (userStatus, blbHandle);
	}


ISC_STATUS API_ROUTINE isc_cancel_blob(ISC_STATUS* userStatus, BlbHandle *blbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->cancelBlob (userStatus, blbHandle);
	}



ISC_STATUS API_ROUTINE isc_put_slice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, USHORT sdlLength, UCHAR* sdl, USHORT paramLength, UCHAR* param, SLONG sliceLength, UCHAR* slice)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->putSlice (userStatus, dbHandle, traHandle, arrayId, sdlLength, sdl, paramLength, param, sliceLength, slice);
	}


ISC_STATUS API_ROUTINE isc_get_slice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, USHORT sdlLength, UCHAR *sdl, USHORT paramLength, UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->getSlice (userStatus, dbHandle, traHandle, arrayId, sdlLength, sdl, paramLength, param, sliceLength, slice, returnLength);
	}



ISC_STATUS API_ROUTINE isc_cancel_events(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->cancelEvents (userStatus, dbHandle, eventId);
	}


ISC_STATUS API_ROUTINE isc_que_events(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId, SSHORT eventsLength, UCHAR* events, FPTR_VOID ast,void* astArg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->queEvents (userStatus, dbHandle, eventId, eventsLength, events, ast, astArg);
	}



ISC_STATUS API_ROUTINE isc_dsql_allocate_statement(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlAllocateStatement (userStatus, dbHandle, dsqlHandle);
	}

ISC_STATUS API_ROUTINE isc_dsql_alloc_statement2(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlAllocateStatement2 (userStatus, dbHandle, dsqlHandle);
	}


ISC_STATUS API_ROUTINE isc_dsql_sql_info(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlSqlInfo (userStatus, dsqlHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE isc_dsql_set_cursor_name(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, char *name, USHORT type)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlSetCursor (userStatus, dsqlHandle, (TEXT*) name, type);
	}


ISC_STATUS API_ROUTINE isc_dsql_prepare (ISC_STATUS* userStatus, 
								   TraHandle *traHandle, 
								   DsqlHandle *dsqlHandle, 
								   USHORT sqlLength, 
								   char *sql, 
								   USHORT dialect,
								   XSQLDA *sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlPrepare (userStatus, traHandle, dsqlHandle, sqlLength, sql, dialect, sqlda);
	}


ISC_STATUS API_ROUTINE isc_dsql_prepare_m(ISC_STATUS* userStatus, 
								   TraHandle *traHandle, 
								   DsqlHandle *dsqlHandle, 
								   USHORT sqlLength, 
								   char *sql, 
								   USHORT dialect, 
								   USHORT itemLength, 
								   char *items, 
								   USHORT bufferLength, 
								   char *buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlPrepare (userStatus, traHandle, dsqlHandle, sqlLength, sql, dialect, itemLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE isc_dsql_describe(ISC_STATUS* userStatus,
										 DsqlHandle *dsqlHandle, 
										 USHORT dialect, 
										 XSQLDA * sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlDescribe (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_describe_bind(ISC_STATUS* userStatus,
											  DsqlHandle *dsqlHandle, 
											  USHORT dialect, 
											  XSQLDA * sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlDescribeBind (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_insert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, SSHORT dialect, XSQLDA *sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlInsert (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_insert_m(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, USHORT blrLength, UCHAR* blr, USHORT msgType, USHORT msgLength, UCHAR* msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlInsert (userStatus, dsqlHandle, blrLength, blr, msgType, msgLength, msg);
	}


ISC_STATUS API_ROUTINE isc_dsql_fetch (ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, USHORT dialect, XSQLDA *sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlFetch (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_fetch_m (ISC_STATUS* userStatus, 
									DsqlHandle *dsqlHandle, 
									USHORT blrLength,
									UCHAR *blr,
									USHORT msgType,
									USHORT msgLength,
									UCHAR *msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlFetch (userStatus, dsqlHandle, blrLength, blr, msgType, msgLength, msg);
	}


ISC_STATUS API_ROUTINE isc_dsql_execute_m (ISC_STATUS* userStatus, 
									DsqlHandle *dsqlHandle, 
									TraHandle *traHandle, 
									USHORT blrLength,
									UCHAR *blr,
									USHORT msgType,
									USHORT msgLength,
									UCHAR *msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, dsqlHandle, traHandle,
								 blrLength, blr, msgType, msgLength, msg,
								 0, NULL, 0, 0, NULL);
	}


ISC_STATUS API_ROUTINE isc_dsql_execute (ISC_STATUS* userStatus, 
											TraHandle *traHandle, 
											DsqlHandle *dsqlHandle, 
											USHORT dialect, 
											XSQLDA *inSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, traHandle, dsqlHandle, dialect, inSqlda, NULL);
	}

ISC_STATUS API_ROUTINE isc_dsql_execute2 (ISC_STATUS* userStatus, 
											TraHandle *traHandle, 
											DsqlHandle *dsqlHandle, 
											USHORT dialect, 
											XSQLDA *inSqlda, 
											XSQLDA *outSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, traHandle, dsqlHandle, dialect, inSqlda, outSqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_execute2_m (ISC_STATUS* userStatus, 
											DsqlHandle *dsqlHandle, 
											TraHandle *traHandle, 
											USHORT inBlrLength, 
											UCHAR* inBlr, 
											USHORT inMsgType, 
											USHORT inMsgLength, 
											UCHAR* inMsg, 
											USHORT outBlrLength, 
											UCHAR* outBlr, 
											USHORT outMsgType, 
											USHORT outMsgLength, 
											UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, dsqlHandle, traHandle,
								 inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
								 outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
	}

ISC_STATUS API_ROUTINE isc_dsql_exec_immed(ISC_STATUS* userStatus,
											DbHandle *dbHandle, 
											TraHandle *traHandle, 
											USHORT sqlLength,
											TEXT *sql,
											USHORT dialect,
											XSQLDA *inSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, inSqlda, NULL);
	}

ISC_STATUS API_ROUTINE isc_dsql_exec_immed2(ISC_STATUS* userStatus,
											DbHandle *dbHandle, 
											TraHandle *traHandle, 
											USHORT sqlLength,
											TEXT *sql,
											USHORT dialect,
											XSQLDA *inSqlda, 
											XSQLDA *outSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, inSqlda, outSqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_execute_immediate (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 XSQLDA *sqda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, sqda, NULL);
	}

ISC_STATUS API_ROUTINE isc_dsql_execute_immediate2 (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 XSQLDA *inSqlda,
											 XSQLDA *outSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inSqlda, outSqlda);
	}

ISC_STATUS API_ROUTINE isc_dsql_execute_immediate_m (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 USHORT inBlrLength, 
											 UCHAR* inBlr, 
											 USHORT inMsgType, 
											 USHORT inMsgLength, 
											 UCHAR* inMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate2 (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
										   0, NULL, 0, 0, NULL);
	}

ISC_STATUS API_ROUTINE isc_dsql_exec_immed2_m (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 USHORT inBlrLength, 
											 UCHAR* inBlr, 
											 USHORT inMsgType, 
											 USHORT inMsgLength, 
											 UCHAR* inMsg, 
											 USHORT outBlrLength, 
											 UCHAR* outBlr, 
											 USHORT outMsgType, 
											 USHORT outMsgLength, 
											 UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate2 (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
										   outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
	}


ISC_STATUS API_ROUTINE isc_dsql_exec_immed3_m (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 USHORT inBlrLength, 
											 UCHAR* inBlr, 
											 USHORT inMsgType, 
											 USHORT inMsgLength, 
											 UCHAR* inMsg, 
											 USHORT outBlrLength, 
											 UCHAR* outBlr, 
											 USHORT outMsgType, 
											 USHORT outMsgLength, 
											 UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate3 (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
										   outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
	}




ISC_STATUS API_ROUTINE isc_dsql_exec_immediate_m(ISC_STATUS* userStatus, 
												 DbHandle *dbHandle, 
												 TraHandle *traHandle, 
												 USHORT sqlLength, 
												 char *sql,
												 USHORT dialect, 
												 USHORT blrLength, 
												 UCHAR *blr, 
												 USHORT msgType, 
												 USHORT msgLength, 
												 UCHAR* msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, 
										  blrLength, blr, msgType, msgLength, msg);
	}



ISC_STATUS API_ROUTINE isc_dsql_free_statement(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, USHORT option)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlFreeStatement (userStatus, dsqlHandle, option);
	}


ISC_STATUS API_ROUTINE isc_cancel_operation (ISC_STATUS* userStatus, DbHandle *dbHandle, USHORT option)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->cancelOperation (userStatus, dbHandle, option);
	}


ISC_STATUS API_ROUTINE isc_service_query(ISC_STATUS* userStatus, 
									SvcHandle *dbHandle, 
									ULONG* reserved,
									USHORT inItemLength, 
									UCHAR* inItem, 
									USHORT outItemLength, 
									UCHAR* outItem, 
									USHORT bufferLength, 
									UCHAR *buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->serviceQuery (userStatus, dbHandle, inItemLength, inItem, outItemLength, outItem, bufferLength, buffer);
	}



ISC_STATUS API_ROUTINE isc_service_attach(ISC_STATUS* userStatus, 
										  USHORT serviceLength, 
										  TEXT *service, 
										  SvcHandle *dbHandle, 
										  USHORT spbLength, 
										  UCHAR *spb)
	{
	if (!dispatch)
		initialize();
		
	JString temp;
	const char *name = service;
	
	if (serviceLength)
		name = temp = truncateName(serviceLength, service);

	return dispatch->serviceAttach (userStatus, name, dbHandle, spbLength, spb, NULL, NULL);
	}

ISC_STATUS API_ROUTINE isc_service_start(ISC_STATUS* userStatus,
										 SvcHandle *dbHandle,
										 ULONG * reserved,
										 USHORT spbLength, 
										 UCHAR * spb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->serviceStart (userStatus, dbHandle, spbLength, spb);
	}

ISC_STATUS API_ROUTINE isc_service_detach(ISC_STATUS* userStatus, SvcHandle *dbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->serviceDetach (userStatus, dbHandle);
	}

ISC_STATUS API_ROUTINE isc_transact_request(ISC_STATUS* userStatus, 
									   DbHandle *dbHandle, 
									   TraHandle *traHandle, 
									   USHORT blrLength, 
									   UCHAR* blr,
									   USHORT inMsgLength, 
									   UCHAR* inMsg, 
									   USHORT outMsgLength, 
									   UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->transactRequest (userStatus, dbHandle, traHandle, blrLength, blr, inMsgLength, inMsg, outMsgLength, outMsg);
	}


ISC_STATUS API_ROUTINE isc_ddl(ISC_STATUS* userStatus, 
							   DbHandle *dbHandle, 
							   TraHandle *traHandle, 
							   USHORT ddlLength, 
							   UCHAR* ddl)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->executeDDL (userStatus, dbHandle, traHandle, ddlLength, ddl);
	}

ISC_STATUS API_ROUTINE isc_seek_blob(ISC_STATUS* userStatus, 
									 BlbHandle *blbHandle,
									 SSHORT mode,
									 SLONG offset,
									 SLONG *result)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->seekBlob (userStatus, blbHandle, mode, offset, result);
	}


ISC_STATUS API_ROUTINE gds__create_database (ISC_STATUS* userStatus, 
									   USHORT fileLength, 
									   const TEXT* fileName, 
									   DbHandle *dbHandle, 
									   USHORT dpbLength, 
									   UCHAR* dpb, 
									   USHORT databaseType)
	{
	return isc_create_database (userStatus, fileLength, fileName, dbHandle, dpbLength, dpb, databaseType);
	}

ISC_STATUS API_ROUTINE gds__attach_database(ISC_STATUS* userStatus, SSHORT fileLength, const TEXT* fileName, DbHandle *dbHandle, SSHORT dpbLength, UCHAR* dpb)
	{
	return isc_attach_database (userStatus, fileLength, fileName, dbHandle, dpbLength, dpb);
	}


ISC_STATUS API_ROUTINE gds__database_info(ISC_STATUS* userStatus, DbHandle *dbHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->databaseInfo (userStatus, dbHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE gds__detach_database(ISC_STATUS* userStatus, DbHandle *dbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->detachDatabase (userStatus, dbHandle);
	}


ISC_STATUS API_ROUTINE gds__drop_database (ISC_STATUS* userStatus, DbHandle *dbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dropDatabase (userStatus, dbHandle);
	}


ISC_STATUS API_ROUTINE gds__start_multiple(ISC_STATUS* userStatus, TraHandle *traHandle, USHORT count, void *stuff)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->startMultiple (userStatus, traHandle, count, (TransactionElement*) stuff);
	}


ISC_STATUS API_ROUTINE_VARARG gds__start_transaction (ISC_STATUS* userStatus, TraHandle *traHandle, SSHORT count, ...)
	{
	TransactionElement vector [32];
	va_list ptr;
	VA_START(ptr, count);

	for (TransactionElement *t = vector, *end = vector + count; t < end; ++t)
		{
		t->dbHandle = va_arg(ptr, DbHandle*);
		t->tpbLength = va_arg(ptr, int);
		t->tpb = va_arg(ptr, UCHAR *);
		}

	return gds__start_multiple (userStatus, traHandle, count, vector);
	}

ISC_STATUS API_ROUTINE gds__reconnect_transaction(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SSHORT length, UCHAR* id)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->reconnectTransaction (userStatus, dbHandle, traHandle, length, id);
	}


ISC_STATUS API_ROUTINE gds__transaction_info(ISC_STATUS* userStatus, TraHandle *traHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->transactionInfo (userStatus, traHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE gds__prepare_transaction(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->prepareTransaction (userStatus, traHandle, 0, 0);
	}


ISC_STATUS API_ROUTINE gds__prepare_transaction2 (ISC_STATUS* userStatus, TraHandle *traHandle, USHORT msgLength, UCHAR* msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->prepareTransaction (userStatus, traHandle, msgLength, msg);
	}


ISC_STATUS API_ROUTINE gds__commit_retaining(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->commitRetaining (userStatus, traHandle);
	}


ISC_STATUS API_ROUTINE gds__commit_transaction(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->commitTransaction (userStatus, traHandle);
	}


ISC_STATUS API_ROUTINE gds__rollback_retaining(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->rollbackRetaining (userStatus, traHandle);
	}


ISC_STATUS API_ROUTINE gds__rollback_transaction(ISC_STATUS* userStatus, TraHandle *traHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->rollbackTransaction (userStatus, traHandle);
	}



ISC_STATUS API_ROUTINE gds__compile_request(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, SSHORT blrLength, UCHAR* blr)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->compileRequest (userStatus, dbHandle, reqHandle, blrLength, blr);
	}

ISC_STATUS API_ROUTINE gds__compile_request2(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, SSHORT blrLength, UCHAR* blr)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->compileRequest2 (userStatus, dbHandle, reqHandle, blrLength, blr);
	}


ISC_STATUS API_ROUTINE gds__start_request (ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->startRequest (userStatus, reqHandle, traHandle, level);
	}

ISC_STATUS API_ROUTINE gds__start_and_send(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle,USHORT msgType, USHORT msgLength, UCHAR *msg, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->startAndSend (userStatus, reqHandle, traHandle, msgType, msgLength, msg, level);
	}


ISC_STATUS API_ROUTINE gds__send (ISC_STATUS* userStatus, ReqHandle *reqHandle, USHORT msgType, USHORT msgLength, UCHAR *msg, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->send (userStatus, reqHandle, msgType, msgLength, msg, level);
	}


ISC_STATUS API_ROUTINE gds__request_info(ISC_STATUS* userStatus, ReqHandle *reqHandle, SSHORT level, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->requestInfo (userStatus, reqHandle, level, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE gds__receive(ISC_STATUS* userStatus, ReqHandle *reqHandle, USHORT msgType, USHORT msgLength, UCHAR *msg, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->receive (userStatus, reqHandle, msgType, msgLength, msg, level);
	}


ISC_STATUS API_ROUTINE gds__unwind_request(ISC_STATUS* userStatus, ReqHandle *reqHandle, SSHORT level)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->unwindRequest (userStatus, reqHandle, level);
	}


ISC_STATUS API_ROUTINE gds__release_request(ISC_STATUS* userStatus, ReqHandle *reqHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->releaseRequest (userStatus, reqHandle);
	}



ISC_STATUS API_ROUTINE gds__create_blob (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid * blobId)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->createBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, 0, NULL);
	}


ISC_STATUS API_ROUTINE gds__create_blob2 (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid * blobId, USHORT bpbLength, UCHAR* bpb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->createBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, bpbLength, bpb);
	}


ISC_STATUS API_ROUTINE gds__open_blob(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->openBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, 0, 0);
	}


ISC_STATUS API_ROUTINE gds__open_blob2(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, USHORT bpbLength, UCHAR* bpb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->openBlob (userStatus, dbHandle, traHandle, blbHandle, blobId, bpbLength, bpb);
	}


ISC_STATUS API_ROUTINE gds__blob_info(ISC_STATUS* userStatus, BlbHandle *blbHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->blobInfo (userStatus, blbHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE gds__put_segment(ISC_STATUS* userStatus, BlbHandle *blbHandle, USHORT segmentLength, UCHAR* segment)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->putSegment (userStatus, blbHandle, segmentLength, segment);
	}


ISC_STATUS API_ROUTINE gds__get_segment (ISC_STATUS* userStatus, BlbHandle *blbHandle, USHORT* segmentLength, USHORT bufferLength, UCHAR* buffer)
	{
	int length;
	ISC_STATUS ret = dispatch->getSegment (userStatus, blbHandle, &length, bufferLength, buffer);

	if (!ret)
		*segmentLength = length;
	
	return ret;
	}


ISC_STATUS API_ROUTINE gds__close_blob (ISC_STATUS* userStatus, BlbHandle *blbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->closeBlob (userStatus, blbHandle);
	}


ISC_STATUS API_ROUTINE gds__cancel_blob(ISC_STATUS* userStatus, BlbHandle *blbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->cancelBlob (userStatus, blbHandle);
	}



ISC_STATUS API_ROUTINE gds__put_slice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, USHORT sdlLength, UCHAR* sdl, USHORT paramLength, UCHAR* param, SLONG sliceLength, UCHAR* slice)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->putSlice (userStatus, dbHandle, traHandle, arrayId, sdlLength, sdl, paramLength, param, sliceLength, slice);
	}


ISC_STATUS API_ROUTINE gds__get_slice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, USHORT sdlLength, UCHAR *sdl, USHORT paramLength, UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->getSlice (userStatus, dbHandle, traHandle, arrayId, sdlLength, sdl, paramLength, param, sliceLength, slice, returnLength);
	}



ISC_STATUS API_ROUTINE gds__cancel_events(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->cancelEvents (userStatus, dbHandle, eventId);
	}


ISC_STATUS API_ROUTINE gds__que_events(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId, SSHORT eventsLength, UCHAR* events, FPTR_VOID ast,void* astArg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->queEvents (userStatus, dbHandle, eventId, eventsLength, events, ast, astArg);
	}



ISC_STATUS API_ROUTINE gds__dsql_allocate_statement(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlAllocateStatement (userStatus, dbHandle, dsqlHandle);
	}

ISC_STATUS API_ROUTINE gds__dsql_alloc_statement2(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlAllocateStatement2 (userStatus, dbHandle, dsqlHandle);
	}


ISC_STATUS API_ROUTINE gds__dsql_sql_info(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, SSHORT itemsLength, char* items, SSHORT bufferLength, char* buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlSqlInfo (userStatus, dsqlHandle, itemsLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE gds__dsql_set_cursor_name(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, char *name, USHORT type)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlSetCursor (userStatus, dsqlHandle, (TEXT*) name, type);
	}


ISC_STATUS API_ROUTINE gds__dsql_prepare (ISC_STATUS* userStatus, 
								   TraHandle *traHandle, 
								   DsqlHandle *dsqlHandle, 
								   USHORT sqlLength, 
								   char *sql, 
								   USHORT dialect)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlPrepare (userStatus, traHandle, dsqlHandle, sqlLength, sql, dialect, 0, NULL, 0, NULL);
	}


ISC_STATUS API_ROUTINE gds__dsql_prepare_m(ISC_STATUS* userStatus, 
								   TraHandle *traHandle, 
								   DsqlHandle *dsqlHandle, 
								   USHORT sqlLength, 
								   char *sql, 
								   USHORT dialect, 
								   USHORT itemLength, 
								   char *items, 
								   USHORT bufferLength, 
								   char *buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlPrepare (userStatus, traHandle, dsqlHandle, sqlLength, sql, dialect, itemLength, (UCHAR*) items, bufferLength, (UCHAR*) buffer);
	}


ISC_STATUS API_ROUTINE gds__dsql_describe(ISC_STATUS* userStatus,
										 DsqlHandle *dsqlHandle, 
										 USHORT dialect, 
										 XSQLDA * sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlDescribe (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_describe_bind(ISC_STATUS* userStatus,
											  DsqlHandle *dsqlHandle, 
											  USHORT dialect, 
											  XSQLDA * sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlDescribeBind (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_insert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, SSHORT dialect, XSQLDA *sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlInsert (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_insert_m(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, USHORT blrLength, UCHAR* blr, USHORT msgType, USHORT msgLength, UCHAR* msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlInsert (userStatus, dsqlHandle, blrLength, blr, msgType, msgLength, msg);
	}


ISC_STATUS API_ROUTINE gds__dsql_fetch (ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, USHORT dialect, XSQLDA *sqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlFetch (userStatus, dsqlHandle, dialect, sqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_fetch_m (ISC_STATUS* userStatus, 
									DsqlHandle *dsqlHandle, 
									USHORT blrLength,
									UCHAR *blr,
									USHORT msgType,
									USHORT msgLength,
									UCHAR *msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlFetch (userStatus, dsqlHandle, blrLength, blr, msgType, msgLength, msg);
	}


ISC_STATUS API_ROUTINE gds__dsql_execute_m (ISC_STATUS* userStatus, 
									DsqlHandle *dsqlHandle, 
									TraHandle *traHandle, 
									USHORT blrLength,
									UCHAR *blr,
									USHORT msgType,
									USHORT msgLength,
									UCHAR *msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, dsqlHandle, traHandle,
								 blrLength, blr, msgType, msgLength, msg,
								 0, NULL, 0, 0, NULL);
	}


ISC_STATUS API_ROUTINE gds__dsql_execute (ISC_STATUS* userStatus, 
											TraHandle *traHandle, 
											DsqlHandle *dsqlHandle, 
											USHORT dialect, 
											XSQLDA *inSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, traHandle, dsqlHandle, dialect, inSqlda, NULL);
	}

ISC_STATUS API_ROUTINE gds__dsql_execute2 (ISC_STATUS* userStatus, 
											TraHandle *traHandle, 
											DsqlHandle *dsqlHandle, 
											USHORT dialect, 
											XSQLDA *inSqlda, 
											XSQLDA *outSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, traHandle, dsqlHandle, dialect, inSqlda, outSqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_execute2_m (ISC_STATUS* userStatus, 
											DsqlHandle *dsqlHandle, 
											TraHandle *traHandle, 
											USHORT inBlrLength, 
											UCHAR* inBlr, 
											USHORT inMsgType, 
											USHORT inMsgLength, 
											UCHAR* inMsg, 
											USHORT outBlrLength, 
											UCHAR* outBlr, 
											USHORT outMsgType, 
											USHORT outMsgLength, 
											UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecute (userStatus, dsqlHandle, traHandle,
								 inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
								 outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
	}

ISC_STATUS API_ROUTINE gds__dsql_exec_immed(ISC_STATUS* userStatus,
											DbHandle *dbHandle, 
											TraHandle *traHandle, 
											USHORT sqlLength,
											TEXT *sql,
											USHORT dialect,
											XSQLDA *inSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, inSqlda, NULL);
	}

ISC_STATUS API_ROUTINE gds__dsql_exec_immed2(ISC_STATUS* userStatus,
											DbHandle *dbHandle, 
											TraHandle *traHandle, 
											USHORT sqlLength,
											TEXT *sql,
											USHORT dialect,
											XSQLDA *inSqlda, 
											XSQLDA *outSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, inSqlda, outSqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_execute_immediate (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT dialect, 
											 char* sql, 
											 USHORT sqlLength, 
											 XSQLDA *sqda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, sqda, NULL);
	}

ISC_STATUS API_ROUTINE gds__dsql_execute_immediate2 (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 XSQLDA *inSqlda,
											 XSQLDA *outSqlda)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inSqlda, outSqlda);
	}

ISC_STATUS API_ROUTINE gds__dsql_execute_immediate_m (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 USHORT inBlrLength, 
											 UCHAR* inBlr, 
											 USHORT inMsgType, 
											 USHORT inMsgLength, 
											 UCHAR* inMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate2 (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
										   0, NULL, 0, 0, NULL);
	}

ISC_STATUS API_ROUTINE gds__dsql_exec_immed2_m (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 USHORT inBlrLength, 
											 UCHAR* inBlr, 
											 USHORT inMsgType, 
											 USHORT inMsgLength, 
											 UCHAR* inMsg, 
											 USHORT outBlrLength, 
											 UCHAR* outBlr, 
											 USHORT outMsgType, 
											 USHORT outMsgLength, 
											 UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate2 (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
										   outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
	}


ISC_STATUS API_ROUTINE gds__dsql_exec_immed3_m (ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DsqlHandle *traHandle, 
											 USHORT sqlLength, 
											 char* sql, 
											 USHORT dialect, 
											 USHORT inBlrLength, 
											 UCHAR* inBlr, 
											 USHORT inMsgType, 
											 USHORT inMsgLength, 
											 UCHAR* inMsg, 
											 USHORT outBlrLength, 
											 UCHAR* outBlr, 
											 USHORT outMsgType, 
											 USHORT outMsgLength, 
											 UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate3 (userStatus, dbHandle, traHandle, sqlLength, sql, dialect,
										   inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
										   outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
	}




ISC_STATUS API_ROUTINE gds__dsql_exec_immediate_m(ISC_STATUS* userStatus, 
												 DbHandle *dbHandle, 
												 TraHandle *traHandle, 
												 USHORT sqlLength, 
												 char *sql,
												 USHORT dialect, 
												 USHORT blrLength, 
												 UCHAR *blr, 
												 USHORT msgType, 
												 USHORT msgLength, 
												 UCHAR* msg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlExecuteImmediate (userStatus, dbHandle, traHandle, sqlLength, sql, dialect, 
										  blrLength, blr, msgType, msgLength, msg);
	}



ISC_STATUS API_ROUTINE gds__dsql_free_statement(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, USHORT option)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->dsqlFreeStatement (userStatus, dsqlHandle, option);
	}


ISC_STATUS API_ROUTINE gds__service_query(ISC_STATUS* userStatus, 
									DbHandle *dbHandle, 
									ULONG* reserved,
									USHORT inItemLength, 
									UCHAR* inItem, 
									USHORT outItemLength, 
									UCHAR* outItem, 
									USHORT bufferLength, 
									UCHAR *buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->serviceQuery (userStatus, dbHandle, inItemLength, inItem, outItemLength, outItem, bufferLength, buffer);
	}



ISC_STATUS API_ROUTINE gds__service_attach(ISC_STATUS* userStatus, 
										  USHORT serviceLength, 
										  TEXT *service, 
										  DbHandle *dbHandle, 
										  USHORT spbLength, 
										  UCHAR *spb)
	{
	//if (!dispatch)
		//initialize();
		
	//return dispatch->serviceAttach (userStatus, service, dbHandle, spbLength, spb);
	return isc_service_attach(userStatus, serviceLength, service, dbHandle, spbLength, spb);
	}

ISC_STATUS API_ROUTINE gds__service_start(ISC_STATUS* userStatus,
										 DbHandle *dbHandle,
										 ULONG * reserved,
										 USHORT spbLength, 
										 UCHAR * spb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->serviceStart (userStatus, dbHandle, spbLength, spb);
	}

ISC_STATUS API_ROUTINE gds__service_detach(ISC_STATUS* userStatus, DbHandle *dbHandle)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->serviceDetach (userStatus, dbHandle);
	}

ISC_STATUS API_ROUTINE gds__transact_request(ISC_STATUS* userStatus, 
									   DbHandle *dbHandle, 
									   TraHandle *traHandle, 
									   USHORT blrLength, 
									   UCHAR* blr,
									   USHORT inMsgLength, 
									   UCHAR* inMsg, 
									   USHORT outMsgLength, 
									   UCHAR* outMsg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->transactRequest (userStatus, dbHandle, traHandle, blrLength, blr, inMsgLength, inMsg, outMsgLength, outMsg);
	}


ISC_STATUS API_ROUTINE gds__ddl(ISC_STATUS* userStatus, 
							   DbHandle *dbHandle, 
							   TraHandle *traHandle, 
							   USHORT ddlLength, 
							   UCHAR* ddl)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->executeDDL (userStatus, dbHandle, traHandle, ddlLength, ddl);
	}

int API_ROUTINE gds__thread_enable(int enable_flag)
	{
	return true;
	}

void API_ROUTINE gds__thread_enter(void)
	{
	}

void API_ROUTINE gds__thread_exit(void)
	{
	}

/***
int API_ROUTINE gds__thread_start(FPTR_INT_VOID_PTR entrypoint,
								  void *arg,
								  int priority, int flags, void *thd_id)
	{
	return 0;
	}
***/

ISC_STATUS API_ROUTINE gds__database_cleanup(ISC_STATUS* userStatus, 
											 DbHandle *dbHandle, 
											 DatabaseCleanupRoutine * routine, 
											 SLONG arg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->databaseCleanup (userStatus, dbHandle, routine, arg);
	}

ISC_STATUS API_ROUTINE gds__transaction_cleanup(ISC_STATUS* userStatus, 
												TraHandle *traHandle, 
												TransactionCleanupRoutine *routine, SLONG arg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->transactionCleanup (userStatus, traHandle, routine, arg);
	}

int API_ROUTINE gds__disable_subsystem(TEXT * subsystem)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->disableSubsystem (subsystem);
	}

int API_ROUTINE gds__enable_subsystem(TEXT * subsystem)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->enableSubsystem (subsystem);
	}

ISC_STATUS API_ROUTINE gds__seek_blob(ISC_STATUS* userStatus, 
									 BlbHandle *blbHandle,
									 SSHORT mode,
									 SLONG offset,
									 SLONG *result)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->seekBlob (userStatus, blbHandle, mode, offset, result);
	}

ISC_STATUS API_ROUTINE gds__event_wait(ISC_STATUS* userStatus,
									   DbHandle *dbHandle,
									   USHORT eventsLength,
									   UCHAR* events, 
									   UCHAR *buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->eventWait (userStatus, dbHandle, eventsLength, events, buffer);
	}

/***
void CVT_move (void)
	{
	}
***/


ISC_STATUS API_ROUTINE isc_database_cleanup(ISC_STATUS * userStatus,
											 DbHandle *handle,
											 DatabaseCleanupRoutine * routine,
											 UCHAR* arg)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->registerCleanupHandler (userStatus, handle, routine, arg);
	}

int API_ROUTINE fb_shutdown_connections (int type, int milliseconds)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->shutdownConnections (type, milliseconds);
	}

ISC_STATUS API_ROUTINE fb_update_account_info (ISC_STATUS* userStatus, 
											   DbHandle *dbHandle,
											   int apbLength,
											   const UCHAR *apb)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->updateAccountInfo (userStatus, dbHandle, apbLength, apb);
	}
											   
ISC_STATUS API_ROUTINE fb_authenticate_user (ISC_STATUS* userStatus, 
										DbHandle *dbHandle,
										int dpbLength, const UCHAR *dpb,
										int infoLength, const UCHAR *info,
										int bufferLength, UCHAR *buffer)
	{
	if (!dispatch)
		initialize();
		
	return dispatch->authenticateUser (userStatus, dbHandle, dpbLength, dpb,
									   infoLength, info,
									   bufferLength, buffer);
	}

} /* extern "C" */
