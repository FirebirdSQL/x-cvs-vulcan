#include <string.h>
#include "firebird.h"
#include "ibase.h"
#include "common.h"
#include "Remote8.h"
#include "inter_proto.h"
#include "utl_proto.h"
#include "isc_f_proto.h"
#include "os/isc_i_proto.h"
#include "MemMgr.h"

MemMgr			defaultMemoryManager;
static Remote8	provider;
static Subsystem* subsystems [] = { &provider, NULL };

extern "C"
{
Subsystem **getSubsystems()
	{
	return subsystems;
	}
} // extern "C"

Remote8::Remote8(void)
{
}

Remote8::~Remote8(void)
{
}

const TEXT* Remote8::getDescription()
{
	return "Remote Interface";
}

ISC_STATUS Remote8::createDatabase (ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  UCHAR* dpb,
									  int databaseType, 
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	//THD_init();
	enterSystem();
	TEXT expandedName [MAXPATHLEN];
	ISC_expand_filename(translatedName, strlen (translatedName), expandedName);
	
	return exitSystem (REM_create_database (statusVector, orgName, expandedName,
											 (RDatabase**) dbHandle, dpbLength, dpb, databaseType,
											 databaseConfiguration, providerConfiguration));
}


ISC_STATUS Remote8::attachDatabase(ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	//THD_init();
	enterSystem();
	TEXT expandedName [MAXPATHLEN];
	ISC_expand_filename(translatedName, strlen (translatedName), expandedName);
	
	return exitSystem (REM_attach_database (statusVector, orgName, 
											 (RDatabase**) dbHandle, dpbLength, dpb, 
											 expandedName, databaseConfiguration, providerConfiguration));
}


ISC_STATUS Remote8::databaseInfo(ISC_STATUS* statusVector, DbHandle *dbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();

	return exitSystem (REM_database_info (statusVector, 
								 (RDatabase**) dbHandle, 
								 itemsLength, items, 
								 bufferLength, buffer));
}


ISC_STATUS Remote8::detachDatabase(ISC_STATUS* statusVector, DbHandle *dbHandle)
{
	enterSystem();

	return exitSystem (REM_detach_database (statusVector, (RDatabase**) dbHandle));
}


ISC_STATUS Remote8::dropDatabase (ISC_STATUS* statusVector, DbHandle *dbHandle)
{
	enterSystem();

	return exitSystem (REM_drop_database (statusVector, (RDatabase**) dbHandle));
}



ISC_STATUS Remote8::startMultiple(ISC_STATUS *statusVector, TraHandle *traHandle, int count, teb *tebs)
{
	enterSystem();

	//return exitSystem (REM_start_transaction (statusVector, (RTransaction**) traHandle, count, tebs));
	return exitSystem (REM_start_transaction (statusVector, (RTransaction**) traHandle, 
											  1, (RDatabase**) tebs->dbHandle, tebs->tpbLength, tebs->tpb));
}


ISC_STATUS Remote8::reconnectTransaction(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, int, UCHAR*)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::transactionInfo(ISC_STATUS* statusVector, TraHandle *traHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::prepareTransaction(ISC_STATUS* statusVector, TraHandle *traHandle, int msgLength, UCHAR *msg)
{
	enterSystem();

	return exitSystem (REM_prepare_transaction (statusVector, (RTransaction**) traHandle, msgLength, msg));
}


ISC_STATUS Remote8::commitRetaining(ISC_STATUS *statusVector, TraHandle *traHandle)
{
	enterSystem();

	return exitSystem (REM_commit_retaining (statusVector, (RTransaction**) traHandle));
}


ISC_STATUS Remote8::commitTransaction(ISC_STATUS* statusVector, TraHandle *traHandle)
{
	enterSystem();

	return exitSystem (REM_commit_transaction (statusVector, (RTransaction**) traHandle));
}


ISC_STATUS Remote8::rollbackRetaining(ISC_STATUS *statusVector, TraHandle *traHandle)
{
	enterSystem();

	return exitSystem (REM_rollback_retaining (statusVector, (RTransaction**) traHandle));
}


ISC_STATUS Remote8::rollbackTransaction(ISC_STATUS *statusVector, TraHandle *traHandle)
{
	enterSystem();

	return exitSystem (REM_rollback_transaction (statusVector, (RTransaction**) traHandle));
}



ISC_STATUS Remote8::compileRequest(ISC_STATUS* statusVector, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	enterSystem();

	return exitSystem (REM_compile_request (statusVector, 
								 (RDatabase**) dbHandle, 
								 (RRequest**) reqHandle, blrLength, blr));
}


ISC_STATUS Remote8::compileRequest2(ISC_STATUS* statusVector, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::startRequest(ISC_STATUS* statusVector, ReqHandle *reqHandle, TraHandle *traHandle, int level)
{
	enterSystem();

	return exitSystem (REM_start_request (statusVector, 
								 (RRequest**) reqHandle, 
								 (RTransaction**) traHandle,
								 level));
}

ISC_STATUS Remote8::startAndSend(ISC_STATUS* statusVector, ReqHandle *reqHandle, TraHandle *traHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	enterSystem();

	return exitSystem (REM_start_and_send (statusVector, 
								 (RRequest**) reqHandle, 
								 (RTransaction**) traHandle,
								 msgType,
								 msgLength,
								 msg,
								 level));
}


ISC_STATUS Remote8::send (ISC_STATUS* statusVector, ReqHandle *reqHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	enterSystem();
	
	return exitSystem (REM_send (statusVector, 
								 (RRequest**) reqHandle, 
								 msgType,
								 msgLength,
								 msg,
								 level));
}


ISC_STATUS Remote8::requestInfo(ISC_STATUS* statusVector, ReqHandle *reqHandle, int level, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();
	
	return exitSystem (REM_request_info (statusVector, 
								 (RRequest**) reqHandle, 
								 level,
								 itemsLength, items,
								 bufferLength, buffer));
}


ISC_STATUS Remote8::receive(ISC_STATUS* statusVector, ReqHandle *reqHandle, int msgType, int msgLength, UCHAR* msg, int level)
{
	enterSystem();

	return exitSystem (REM_receive (statusVector, 
								 (RRequest**) reqHandle, 
								 msgType, msgLength, msg, level));
}


ISC_STATUS Remote8::unwindRequest(ISC_STATUS* statusVector, ReqHandle *reqHandle, int level)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::releaseRequest(ISC_STATUS*statusVector, ReqHandle *reqHandle)
{
	enterSystem();

	return exitSystem (REM_release_request (statusVector, (RRequest**) reqHandle));
}



ISC_STATUS Remote8::createBlob (ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	enterSystem();
	
	return exitSystem (REM_create_blob2 (statusVector, 
										(RDatabase**) dbHandle, 
										(RTransaction**) traHandle,
										(RBlob**) blbHandle,
										blobId,
										bpbLength,
										bpb));
}


ISC_STATUS Remote8::openBlob(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	enterSystem();
	
	return exitSystem (REM_open_blob2 (statusVector, 
										(RDatabase**) dbHandle, 
										(RTransaction**) traHandle,
										(RBlob**) blbHandle,
										blobId,
										bpbLength,
										bpb));
}


ISC_STATUS Remote8::blobInfo(ISC_STATUS* statusVector, BlbHandle *blbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();
	
	return exitSystem (REM_blob_info (statusVector, (RBlob**) blbHandle, 
									  itemsLength, items,
									  bufferLength, buffer));
}


ISC_STATUS Remote8::putSegment(ISC_STATUS* statusVector, BlbHandle *blbHandle, int segmentLength, UCHAR* segment)
{
	enterSystem();
	
	return exitSystem (REM_put_segment (statusVector, (RBlob**) blbHandle, segmentLength, segment));
}


ISC_STATUS Remote8::getSegment (ISC_STATUS* statusVector, BlbHandle *blbHandle, int* segmentLength, int bufferLength, UCHAR* buffer)
{
	enterSystem();
	
	return exitSystem (REM_get_segment (statusVector, (RBlob**) blbHandle, segmentLength, bufferLength, buffer));
}


ISC_STATUS Remote8::seekBlob (ISC_STATUS* userStatus, 
							 BlbHandle *blbHandle,
							 int mode,
							 SLONG offset,
							 SLONG *result)
{
	enterSystem();
	
	return exitSystem (REM_seek_blob (userStatus, (RBlob**) blbHandle, mode, offset, result));
}

ISC_STATUS Remote8::closeBlob (ISC_STATUS* statusVector, BlbHandle *blbHandle)
{
	enterSystem();
	
	return exitSystem (REM_close_blob (statusVector, (RBlob**) blbHandle));
}


ISC_STATUS Remote8::cancelBlob(ISC_STATUS* statusVector, BlbHandle *blbHandle)
{
	enterSystem();
	
	return exitSystem (REM_cancel_blob (statusVector, (RBlob**) blbHandle));
}



ISC_STATUS Remote8::putSlice(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR* sdl, int paramLength, UCHAR* param, SLONG sliceLength, UCHAR* slice)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::getSlice(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR *sdl, int paramLength, UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
{
	return entrypointUnavailable (statusVector);
}



ISC_STATUS Remote8::cancelEvents(ISC_STATUS* statusVector, DbHandle *dbHandle, SLONG* eventId)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::queEvents(ISC_STATUS* statusVector, DbHandle *dbHandle, SLONG* eventId, int eventsLength, UCHAR* events, FPTR_VOID ast,void* astArg)
{
	return entrypointUnavailable (statusVector);
}



ISC_STATUS Remote8::dsqlAllocateStatement(ISC_STATUS* statusVector, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	enterSystem();

	return exitSystem (REM_allocate_statement (statusVector, (RDatabase**) dbHandle, (RStatement**) dsqlHandle));
}


ISC_STATUS Remote8::dsqlAllocateStatement2(ISC_STATUS* statusVector, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlSqlInfo(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	enterSystem();

	return exitSystem (REM_sql_info (statusVector, (RStatement**) dsqlHandle, itemsLength, items, bufferLength, buffer));
}


ISC_STATUS Remote8::dsqlSetCursor(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, const TEXT* cursorName, int cursorType)
{
	enterSystem();

	return exitSystem (REM_set_cursor_name (statusVector, (RStatement**) dsqlHandle, cursorName, cursorType));
}


ISC_STATUS Remote8::dsqlPrepare(ISC_STATUS* statusVector, TraHandle *traHandle, DsqlHandle *dsqlHandle, 
								int sqlLength, const TEXT *sql, int dialect, int itemLength, const UCHAR *items, 
								int bufferLength, UCHAR *buffer)
{
	enterSystem();

	return exitSystem (REM_prepare (statusVector, (RTransaction**) traHandle, (RStatement**) dsqlHandle,
											  sqlLength, sql, dialect, itemLength, items,
											  bufferLength, buffer));
}


ISC_STATUS Remote8::dsqlDescribe(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlDescribeBind(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	return entrypointUnavailable (statusVector);
	enterSystem();

	//return exitSystem (REM_describe_bind (statusVector, (RStatement**) dsqlHandle, dialect, sqlda));
}


ISC_STATUS Remote8::dsqlInsert(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlInsert(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int blrLength, UCHAR* blr, int msgType, int msgLength, const UCHAR* msg)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlFetch(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlFetch(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, 
							  int blrLength, const UCHAR* blr, 
							  int msgType, int msgLength, UCHAR* msg)
{
	enterSystem();

	return exitSystem (REM_fetch (statusVector,
								  (RStatement**) dsqlHandle,
								  blrLength, blr,
								  msgType, msgLength, msg));
}


ISC_STATUS Remote8::dsqlExecuteImmediate (ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
								 int sqlLength, const char *sql, int dialect,
								 XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlExecute (ISC_STATUS* statusVector, TraHandle *traHandle, DsqlHandle *dsqlHandle,
								 int inBlrLength, UCHAR *inBlr, 
								 int inMsgType, int inMsgLength, UCHAR *inMsg, 
								 int outBlrLength, UCHAR *outBlr, 
								 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	enterSystem();

	return exitSystem (REM_execute2 (statusVector,
									(RTransaction**) traHandle,
									(RStatement**) dsqlHandle,
									inBlrLength, inBlr, 
									inMsgType, inMsgLength, inMsg, 
									outBlrLength, outBlr, 
									outMsgType, outMsgLength, outMsg));
}


ISC_STATUS Remote8::dsqlExecute (ISC_STATUS* statusVector, TraHandle *traHandle, DsqlHandle *dsqlHandle, int dialect, XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	return entrypointUnavailable (statusVector);
}


//ISC_STATUS Remote8::dsqlExecute2(ISC_STATUS* statusVector, TraHandle *traHandle, DsqlHandle *dsqlHandle, int, UCHAR*,int, int, UCHAR*, int, UCHAR*, int, int,UCHAR*)


ISC_STATUS Remote8::dsqlExecuteImmediate2(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, UCHAR *inMsg, 
										 int outBlrLength, UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return exitSystem (REM_execute_immediate2 (statusVector,
									(RDatabase**) dbHandle, 
									(RTransaction**) traHandle,
									sqlLength, sql, dialect,
									inBlrLength, inBlr, 
									inMsgType, inMsgLength, inMsg, 
									outBlrLength, outBlr, 
									outMsgType, outMsgLength, outMsg));
}


ISC_STATUS Remote8::dsqlExecuteImmediate3(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, UCHAR *inMsg, 
										 int outBlrLength, UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlExecuteImmediate (ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, int blrLength, UCHAR *blr, 
										 int msgType, int msgLength, UCHAR* msg)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::dsqlFreeStatement(ISC_STATUS* statusVector, DsqlHandle *dsqlHandle, int options)
{
	enterSystem();

	return exitSystem (REM_free_statement (statusVector, (RStatement**) dsqlHandle, options));
}



//ISC_STATUS Remote8::cancelOperation(ISC_STATUS* statusVector, DbHandle *dbHandle, int);
ISC_STATUS Remote8::serviceQuery(ISC_STATUS *statusVector, 
								SvcHandle *dbHandle, 
								int inItemLength, 
								UCHAR* inItem, 
								int outItemLength, 
								UCHAR* outItem, 
								int bufferLength, 
								UCHAR *buffer)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::serviceDetach(ISC_STATUS *statusVector, SvcHandle *dbHandle)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::serviceAttach(ISC_STATUS *statusVector, 
								  const TEXT *service, 
								  SvcHandle *dbHandle, 
								  int spbLength, 
								  UCHAR *spb, 
								  ConfObject* servicesConfiguration,
								  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::serviceStart(ISC_STATUS* statusVector,
								 SvcHandle *dbHandle,
								 int spbLength, 
								 UCHAR * spb)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::transactRequest(ISC_STATUS* statusVector, 
								   DbHandle *dbHandle, 
								   TraHandle *traHandle, 
								   int blrLength, 
								   UCHAR* blr,
								   int inMsgLength, 
								   UCHAR* inMsg, 
								   int outMsgLength, 
								   UCHAR* outMsg)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::executeDDL(ISC_STATUS* statusVector, DbHandle *dbHandle, TraHandle *traHandle, int ddlLength, UCHAR* ddl)
{
	return entrypointUnavailable (statusVector);
}


int Remote8::enableSubsystem (TEXT* subSystem)
{
	return false;
}


int Remote8::disableSubsystem (TEXT* subSystem)
{
	return false;
}


ISC_STATUS Remote8::databaseCleanup (ISC_STATUS* statusVector, 
									 DbHandle *dbHandle, 
									 DatabaseCleanupRoutine *routine, 
									 SLONG arg)
{
	return entrypointUnavailable (statusVector);
}


ISC_STATUS Remote8::transactionCleanup (ISC_STATUS* statusVector, 
									   TraHandle *traHandle, 
									   TransactionCleanupRoutine *routine, 
									   SLONG arg)
{
	return entrypointUnavailable (statusVector);
}



ISC_STATUS Remote8::eventWait(ISC_STATUS* statusVector,
							 DbHandle *dbHandle,
							 int eventsLength,
							 UCHAR* events, 
							 UCHAR *buffer)
{
	return entrypointUnavailable (statusVector);
}


void Remote8::enterSystem(void)
{
	ISC_enter();
}

ISC_STATUS Remote8::exitSystem(ISC_STATUS code)
{
	ISC_exit();

	return code;
}
