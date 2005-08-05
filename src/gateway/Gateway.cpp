#include "firebird.h"
#include "common.h"
#include "Gateway.h"
#include "iberror.h"
#include "ExternalLibrary.h"

static Gateway provider ("fbclient");
static Subsystem* subsystems [] = { &provider, NULL };
extern "C" Subsystem **getSubsystems() { return subsystems; };

#define ENTRY(fn,sig) \
	if (!entrypoints[fn]) entrypoints[fn] = externalLibrary->getEntryPoint (#fn); \
	if (entrypoints[fn]) return ((ISC_STATUS (__stdcall *)sig) entrypoints[fn])


Gateway::Gateway(const char *libraryName)
{
	externalLibrary = ExternalLibrary::loadLibrary (libraryName);
	memset (entrypoints, 0, sizeof (entrypoints));
}

Gateway::~Gateway(void)
{
	if (externalLibrary)
		delete externalLibrary;
}

const TEXT* Gateway::getDescription()
{
	return "Generic OSRI Gateway";
}

ISC_STATUS Gateway::createDatabase (ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  UCHAR* dpb,
									  int databaseType, 
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (statusVector);
}

	
ISC_STATUS Gateway::attachDatabase(ISC_STATUS* userStatus, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpb_length, 
									  UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	ENTRY (isc_attach_database, (ISC_STATUS*, int, const char*, void*, int, const UCHAR*))
			(userStatus, strlen (translatedName), translatedName, dbHandle, dpb_length, dpb);
			
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::databaseInfo(ISC_STATUS* userStatus, DbHandle *dbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::detachDatabase(ISC_STATUS* userStatus, DbHandle *dbHandle)
{
	ENTRY (isc_detach_database, (ISC_STATUS*, void*))
			(userStatus, dbHandle);

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dropDatabase (ISC_STATUS* userStatus, DbHandle *dbHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::startMultiple(ISC_STATUS *userStatus, TraHandle *traHandle, int count, teb *tebs)
{
	ENTRY (isc_start_multiple, (ISC_STATUS*, void*, int, void*))
			(userStatus, traHandle, count, tebs);

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::reconnectTransaction(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int, UCHAR*)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::transactionInfo(ISC_STATUS* userStatus, TraHandle *traHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::prepareTransaction(ISC_STATUS* userStatus, TraHandle *traHandle, int, UCHAR*)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::commitRetaining(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::commitTransaction(ISC_STATUS* userStatus, TraHandle *traHandle)
{
	ENTRY (isc_commit_transaction, (ISC_STATUS*, void*))
			(userStatus, traHandle);

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::rollbackRetaining(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::rollbackTransaction(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::compileRequest(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	ENTRY (isc_compile_request, (ISC_STATUS*, void*, void*, int, const UCHAR*))
			(userStatus, dbHandle, reqHandle, blrLength, blr);

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::compileRequest2(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::startRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int level)
{
	ENTRY (isc_start_request, (ISC_STATUS*, void*, void*, int))
			(userStatus, reqHandle, traHandle, level);

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::startAndSend(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::send (ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::requestInfo(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::receive(ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, UCHAR* msg, int level)
{
	ENTRY (isc_receive, (ISC_STATUS*, void*, int, int, UCHAR*, int))
			(userStatus, reqHandle, msgType, msgLength, msg, level);

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::unwindRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::releaseRequest(ISC_STATUS*userStatus, ReqHandle *reqHandle)
{
	ENTRY (isc_release_request, (ISC_STATUS*, void*))
			(userStatus, reqHandle);

	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::createBlob (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::openBlob(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::blobInfo(ISC_STATUS* userStatus, BlbHandle *blbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::putSegment(ISC_STATUS* userStatus, BlbHandle *blbHandle, int segmentLength, UCHAR* segment)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::getSegment (ISC_STATUS* userStatus, BlbHandle *blbHandle, int* segmentLength, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::closeBlob (ISC_STATUS* userStatus, BlbHandle *blbHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::cancelBlob(ISC_STATUS* userStatus, BlbHandle *blbHandle)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::putSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR* sdl, int paramLength, UCHAR* param, SLONG sliceLength, UCHAR* slice)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::getSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, SLONG* arrayId, int sdlLength, UCHAR *sdl, int paramLength, UCHAR *param, SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::cancelEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::queEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId, int eventsLength, UCHAR* events, FPTR_VOID ast,void* astArg)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::dsqlAllocateStatement(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlAllocateStatement2(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlSqlInfo(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlSetCursor(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, const TEXT* cursorName, int cursorType)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlPrepare(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int sqlLength, const TEXT *sql, int dialect, int itemLength, const UCHAR *items, int bufferLength, UCHAR *buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlDescribe(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlDescribeBind(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, UCHAR* blr, int msgType, int msgLength, const UCHAR* msg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, const UCHAR* blr, int msgType, int msgLength, UCHAR* msg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
								 int sqlLength, const char *sql, int dialect,
								 XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, 
								 int inBlrLength, UCHAR *inBlr, 
								 int inMsgType, int inMsgLength, UCHAR *inMsg, 
								 int outBlrLength, UCHAR *outBlr, 
								 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int dialect, XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	return entrypointUnavailable (userStatus);
}


//ISC_STATUS Gateway::dsqlExecute2(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int, UCHAR*,int, int, UCHAR*, int, UCHAR*, int, int,UCHAR*)


ISC_STATUS Gateway::dsqlExecuteImmediate2(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, UCHAR *inMsg, 
										 int outBlrLength, UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlExecuteImmediate3(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, UCHAR *inMsg, 
										 int outBlrLength, UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, int blrLength, UCHAR *blr, 
										 int msgType, int msgLength, UCHAR* msg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::dsqlFreeStatement(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int option)
{
	return entrypointUnavailable (userStatus);
}



//ISC_STATUS Gateway::cancelOperation(ISC_STATUS* userStatus, DbHandle *dbHandle, int);
ISC_STATUS Gateway::serviceQuery(ISC_STATUS *userStatus, 
								DbHandle *SvcHandle, 
								int inItemLength, 
								UCHAR* inItem, 
								int outItemLength, 
								UCHAR* outItem, 
								int bufferLength, 
								UCHAR *buffer)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::serviceDetach(ISC_STATUS *userStatus, SvcHandle *dbHandle)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::serviceAttach(ISC_STATUS *userStatus, 
								  const TEXT *service, 
								  SvcHandle *dbHandle, 
								  int spbLength, 
								  UCHAR *spb, 
								  ConfObject* servicesConfiguration,
								  ConfObject* providerConfiguration)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::serviceStart(ISC_STATUS* userStatus,
								 SvcHandle *dbHandle,
								 int spbLength, 
								 UCHAR * spb)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::transactRequest(ISC_STATUS* userStatus, 
								   DbHandle *dbHandle, 
								   TraHandle *traHandle, 
								   int blrLength, 
								   UCHAR* blr,
								   int inMsgLength, 
								   UCHAR* inMsg, 
								   int outMsgLength, 
								   UCHAR* outMsg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::executeDDL(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int ddlLength, UCHAR* ddl)
{
	return entrypointUnavailable (userStatus);
}


int Gateway::enableSubsystem (TEXT* subSystem)
{
	return false;
}


int Gateway::disableSubsystem (TEXT* subSystem)
{
	return false;
}


ISC_STATUS Gateway::databaseCleanup (ISC_STATUS* userStatus, 
									 DbHandle *dbHandle, 
									 DatabaseCleanupRoutine *routine, 
									 SLONG arg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::transactionCleanup (ISC_STATUS* userStatus, 
									   TraHandle *traHandle, 
									   TransactionCleanupRoutine *routine, 
									   SLONG arg)
{
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Gateway::seekBlob (ISC_STATUS* userStatus, 
							 BlbHandle *blbHandle,
							 int mode,
							 SLONG offset,
							 SLONG *result)
{
	return entrypointUnavailable (userStatus);
}



ISC_STATUS Gateway::eventWait(ISC_STATUS* userStatus,
							 DbHandle *dbHandle,
							 int eventsLength,
							 UCHAR* events, 
							 UCHAR *buffer)
{
	return entrypointUnavailable (userStatus);
}

