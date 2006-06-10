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

// Dispatch.cpp: implementation of the Dispatch class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fbdev.h"
#include "ibase.h"
#include "common.h"
#include "../jrd/dsc.h"
#include "Dispatch.h"
#include "SubsysHandle.h"
#include "StatusVector.h"
#include "YTransaction.h"
#include "YRequest.h"
#include "YStatement.h"
#include "YService.h"
#include "YBlob.h"
#include "YSQLDA.h"
#include "Provider.h"
#include "Configuration.h"
#include "ConfObject.h"
#include "HandleManager.h"
#include "SpecialSql.h"
#include "OSRIException.h"
#include "Element.h"
#include "PBGen.h"
#include "AdminException.h"
#include "Parameters.h"

#define SET_HANDLE(type,handle,value)			*((type*) handle)= (type) value

static const int MAX_ITERATIONS = 10;

/* Information items for DSQL prepare */

static const UCHAR prepareInfo[] =
	{
	isc_info_sql_select,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
	};

/* Information items for SQL info */

static const UCHAR selectInfo[] =
	{
	isc_info_sql_select,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
	};

static const UCHAR bindInfo[] =
	{
	isc_info_sql_bind,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
	};

//typedef Subsystem** (GetSubsystems)();

//Dispatch				dispatch(0);


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Dispatch::Dispatch(int flags)
{
	providers = NULL;
	shutdownLevel = 0;
	traceFlags = flags;
	//traceFlags |= 6;
	initialized = false;
}

Dispatch::~Dispatch()
{
	for (Provider *provider; provider = providers;)
		{
		providers = provider->next;
		delete provider;
		}
}

ISC_STATUS Dispatch::createDatabase (ISC_STATUS* userStatus, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  const UCHAR* dpb,
									  int databaseType, 
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	if (!initialized)
		initialize();
		
	if (traceFlags & traceSQL)
		printTrace ("createDatabase %s", translatedName);
	else
		trace ("createDatabase %s", translatedName);

	StatusVector statusVector (userStatus, traceFlags);

	if (*((isc_db_handle*) dbHandle))
		return statusVector.postAndReturn (isc_bad_db_handle);

	if (!orgName)
		{
		statusVector.post(isc_bad_db_format, isc_arg_string, translatedName, isc_arg_end);
		return statusVector.getReturn();
		}

	if (!translatedName)
		translatedName = orgName;

	if (dpbLength > 0 && !dpb)
		return statusVector.postAndReturn (isc_bad_dpb_form);

	JString dbName = translatedName;
	JString prior;
	UCHAR *extendedDpb = (UCHAR*) dpb;
	int extendedDpbLength = rewriteDpb (dpbLength, &extendedDpb);
	bool hasProvider = false;
	bool haveError = false;
	
	for (int iteration = 0; !hasProvider && iteration < MAX_ITERATIONS; ++iteration)
		{
		ConfObject *confObject = Configuration::findObject ("database", dbName);
		
		if (!confObject)
			{
			statusVector.post(isc_bad_db_format, isc_arg_string, translatedName, isc_arg_end);
			return statusVector.getReturn();
			}
		
		dbName = confObject->getValue ("filename", (const char*) dbName);
		
		if (dbName == prior)
			break;
			
		prior = dbName;
		
		for (int n = 0;; ++n)
			{
			JString providerName = confObject->getValue (n, "provider");
			if (providerName.IsEmpty())
				break;
			hasProvider = true;
			Provider *provider = getProvider (providerName);
			if (provider->subsystems)
				for (int subsys = 0;; ++subsys)
					{
					Subsystem *subsystem = provider->subsystems [subsys];
					if (!subsystem)
						break;
					DbHandle tempHandle = NULL;
					if (subsystem->createDatabase (statusVector, orgName, dbName, &tempHandle, 
												   extendedDpbLength, extendedDpb, databaseType, 
												   confObject, provider->configuration))
						haveError = true;
					else
						{
						SubsysHandle *handle = new SubsysHandle (subsystem, tempHandle);
						*((isc_db_handle*) dbHandle) = (isc_db_handle) databaseHandles.allocateHandle (handle);
						if (extendedDpb != dpb)
							delete [] extendedDpb;
						return statusVector.getCode();
						}
					}
			}
		}

	if (extendedDpb != dpb)
		delete [] extendedDpb;
		
	if (haveError)
		return statusVector.getCode();

	return entrypointUnavailable (userStatus);
}


ISC_STATUS Dispatch::attachDatabase(ISC_STATUS* userStatus, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  DbHandle *dbHandle, 
									  int dpbLength, 
									  const UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration)
{
	if (!initialized)
		initialize();
		
	if (traceFlags & traceSQL)
		printTrace ("attachDatabase %s", translatedName);
	else
		trace ("attachDatabase %s", translatedName);
	
	StatusVector localVector (userStatus, traceFlags);
	StatusVector *statusVector = &localVector;
	ISC_STATUS	localStatus[ISC_STATUS_LENGTH];

	if (*((isc_db_handle*) dbHandle))
		return statusVector->postAndReturn (isc_bad_db_handle);

	if (!translatedName)
		{
		statusVector->post(isc_bad_db_format, isc_arg_string, translatedName, isc_arg_end);
		return statusVector->getReturn();
		}

	if (dpbLength > 0 && !dpb)
		return statusVector->postAndReturn (isc_bad_dpb_form);

	JString dbName = translatedName;
	JString prior;
	UCHAR *extendedDpb = (UCHAR*) dpb;
	int extendedDpbLength = rewriteDpb (dpbLength, &extendedDpb);
	bool hasProvider = false;
	bool haveError = false;
	
	for (int iteration = 0; !hasProvider && iteration < MAX_ITERATIONS; ++iteration)
		{
		ConfObject *confObject;
		try
			{
			confObject = Configuration::findObject ("database", dbName);
			
			if (!confObject)
				{
				statusVector->post(isc_bad_db_format, isc_arg_string, translatedName, isc_arg_end);
				return statusVector->getReturn();
				}
			}
		catch (AdminException& exception)
			{
			exception;
			statusVector->post(isc_bad_db_format, isc_arg_string, translatedName, isc_arg_end);
			return statusVector->getReturn();
			}
		
		dbName = confObject->getValue ("filename", (const char*) dbName);
		/***
		if (dbName == prior)
			break;
		***/
		prior = dbName;
		
		for (int n = 0;; ++n)
			{
			JString providerName = confObject->getValue (n, "provider");
			if (providerName.IsEmpty())
				break;
			hasProvider = true;
			Provider *provider = getProvider (providerName);
			if (provider->subsystems)
				for (int subsys = 0;; ++subsys)
					{
					Subsystem *subsystem = provider->subsystems [subsys];
					if (!subsystem)
						break;
					DbHandle tempHandle = NULL;
					if (subsystem->attachDatabase (*statusVector, orgName, dbName, &tempHandle, 
												   extendedDpbLength, extendedDpb, 
												   confObject, provider->configuration))
						{
						if (!haveError)
							memcpy (localStatus, statusVector->statusVector, sizeof(localStatus));

						haveError = true;
						}
					else
						{
						SubsysHandle *handle = new SubsysHandle (subsystem, tempHandle);
						*((isc_db_handle*) dbHandle) = (isc_db_handle) databaseHandles.allocateHandle (handle);
						if (extendedDpb != dpb)
							delete [] extendedDpb;
						return statusVector->getCode();
						}
					}
			}
		}

	if (extendedDpb != dpb)
		delete [] extendedDpb;

	if (haveError)
		{
		memcpy (statusVector->statusVector, localStatus, sizeof(localStatus));
		return statusVector->getReturn();
		}
		
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Dispatch::databaseInfo(ISC_STATUS* userStatus, DbHandle *dbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	trace ("databaseInfo");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	handle->subsystem->databaseInfo (statusVector, &handle->handle, itemsLength, items, bufferLength, buffer);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::detachDatabase(ISC_STATUS* userStatus, DbHandle *dbHandle)
{
	if (traceFlags & traceSQL)
		printTrace ("detachDatabase");
	else
		trace ("detachDatabase");
		
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	handle->releaseRequests();
    handle->releaseStatements();

	if (!handle->subsystem->detachDatabase (statusVector, &handle->handle))
		{
		int slot = getHandleSlot (dbHandle);
		databaseHandles.releaseHandle (slot);
		*((isc_db_handle*) dbHandle) = 0;
		delete handle;
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dropDatabase (ISC_STATUS* userStatus, DbHandle *dbHandle)
{
	trace ("dropDatabase");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	handle->releaseRequests();

	if (!handle->subsystem->dropDatabase (
			statusVector, 
			&handle->handle))
		{
		*((isc_db_handle*)dbHandle) = 0;
		delete handle;
		}

	return statusVector.getReturn();
}



ISC_STATUS Dispatch::startMultiple(ISC_STATUS *userStatus, TraHandle *traHandle, int count, const TransactionElement *tebs)
{
	trace ("startMultiple");
	StatusVector statusVector (userStatus, traceFlags);

	if (*(isc_tr_handle*) traHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	YTransaction *transaction = new YTransaction (count);

	for (int n = 0; n < count; ++n)
		{
		const TransactionElement *element = tebs + n;
		SubsysHandle *subsystem = getDatabase (element->dbHandle);
		if (!subsystem)
			{
			statusVector.postCode (isc_bad_db_handle);
			break;
			}
		if (element->tpbLength && !element->tpb)
			{
			statusVector.postCode (isc_bad_tpb_form);
			break;
			}
		transaction->setDatabase (n, subsystem, element->tpbLength, element->tpb);
		}
		
	if (statusVector.success() && !transaction->start (statusVector))
		SET_HANDLE (isc_tr_handle, traHandle, transactionHandles.allocateHandle (transaction));
	else
		delete transaction;

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::reconnectTransaction(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int infoLength, const UCHAR *info)
{
	trace ("reconnectTransaction");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = new YTransaction (1);

	if (!transaction->reconnect (statusVector, handle, infoLength, info))
		SET_HANDLE (isc_tr_handle, traHandle, transactionHandles.allocateHandle (transaction));
	else
		delete transaction;
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::transactionInfo(ISC_STATUS* userStatus, TraHandle *traHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	trace ("transactionInfo");
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	transaction->transactionInfo(statusVector, itemsLength, items, bufferLength, buffer);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::prepareTransaction(ISC_STATUS* userStatus, TraHandle *traHandle, int msgLength, const UCHAR* msg)
{
	trace ("prepareTransaction");
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	transaction->prepare(statusVector, msgLength, msg);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::commitRetaining(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	trace ("commitRetaining");
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	transaction->commitRetaining(statusVector);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::commitTransaction(ISC_STATUS* userStatus, TraHandle *traHandle)
{
	trace ("commitTransaction");
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (!transaction->commit(statusVector))
		{
		int slot = getHandleSlot (traHandle);
		transactionHandles.releaseHandle (slot);
		*((isc_tr_handle*) traHandle) = 0;
		delete transaction;
		}
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::rollbackRetaining(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	trace ("rollbackRetaining");
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	transaction->rollbackRetaining(statusVector);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::rollbackTransaction(ISC_STATUS *userStatus, TraHandle *traHandle)
{
	trace ("rollbackTransaction");
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (!transaction->rollback(statusVector))
		{
		int slot = getHandleSlot (traHandle);
		transactionHandles.releaseHandle (slot);
		*((isc_tr_handle*) traHandle) = 0;
		delete transaction;
		}
	
	return statusVector.getReturn();
}



ISC_STATUS Dispatch::compileRequest(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	trace ("compileRequest");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	if (*(isc_req_handle*) reqHandle)
		return statusVector.postAndReturn (isc_bad_req_handle);

	DbHandle tempHandle = NULL;

	if (!handle->subsystem->compileRequest (
			statusVector, 
			&handle->handle, 
			&tempHandle, 
			blrLength, 
			blr))
		{
		YRequest *requestHandle = new YRequest (handle, tempHandle);
		*((isc_req_handle*) reqHandle) = (isc_req_handle) requestHandles.allocateHandle (requestHandle);
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::compileRequest2(ISC_STATUS* userStatus, DbHandle *dbHandle, ReqHandle *reqHandle, int blrLength, const UCHAR* blr)
{
	trace ("compileRequest2");
	//DbHandle *orgPointer = dbHandle;
	ISC_STATUS ret = compileRequest (userStatus, dbHandle, reqHandle, blrLength, blr);

	if (!ret)
		{
		YRequest *request = getRequest (reqHandle);
		request->userPtr = reqHandle; //orgPointer;
		}

	return ret;
}


ISC_STATUS Dispatch::startRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int level)
{
	trace ("startRequest");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);
	YTransaction *transaction = getTransaction (traHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (request->subsystem);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	request->subsystem->subsystem->startRequest (
			statusVector, 
			&request->handle, 
			&trHandle,
			level);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::startAndSend(ISC_STATUS* userStatus, ReqHandle *reqHandle, TraHandle *traHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	trace ("startAndSend");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);
	YTransaction *transaction = getTransaction (traHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (request->subsystem);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	request->subsystem->subsystem->startAndSend (
			statusVector, 
			&request->handle, 
			&trHandle,
			msgType,
			msgLength,
			msg,
			level);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::send (ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, const UCHAR* msg, int level)
{
	trace ("send");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	request->subsystem->subsystem->send (
			statusVector, 
			&request->handle, 
			msgType,
			msgLength,
			msg,
			level);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::requestInfo(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	trace ("requestInfo");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	request->subsystem->subsystem->requestInfo (statusVector,
									&request->handle,
									level,
									itemsLength,
									items,
									bufferLength,
									buffer);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::receive(ISC_STATUS* userStatus, ReqHandle *reqHandle, int msgType, int msgLength, UCHAR* msg, int level)
{
	trace ("receive");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	request->subsystem->subsystem->receive (
			statusVector, 
			&request->handle, 
			msgType,
			msgLength,
			msg,
			level);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::unwindRequest(ISC_STATUS* userStatus, ReqHandle *reqHandle, int level)
{
	trace ("unwindRequest");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	request->subsystem->subsystem->unwindRequest(
			statusVector, 
			&request->handle, 
			level);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::releaseRequest(ISC_STATUS*userStatus, ReqHandle *reqHandle)
{
	trace ("releaseRequest");
	StatusVector statusVector (userStatus, traceFlags);
	YRequest *request = getRequest (reqHandle);

	if (!request)
		return statusVector.postAndReturn (isc_bad_req_handle);

	isc_req_handle handleCopy = *(isc_req_handle*) reqHandle;
	
	if (!request->releaseRequest( statusVector))
		{
		*(isc_req_handle*) reqHandle = 0;
		delete request;
		requestHandles.releaseHandle(handleCopy);
		}

	return statusVector.getReturn();
}



ISC_STATUS Dispatch::createBlob (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	trace ("createBlob");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (handle);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (*(isc_blob_handle*) blbHandle)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	BlbHandle tempHandle = NULL;

	if (!handle->subsystem->createBlob (
			statusVector, 
			&handle->handle,
			&trHandle,
			&tempHandle, 
			blobId,
			bpbLength, 
			bpb))
		{
		YBlob *blob = new YBlob (handle, tempHandle);
		*(isc_blob_handle*) blbHandle = (isc_blob_handle) blobHandles.allocateHandle (blob);
		}
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::openBlob(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, BlbHandle *blbHandle, bid* blobId, int bpbLength, const UCHAR* bpb)
{
	trace ("openBlob");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (handle);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (*(isc_blob_handle*) blbHandle)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	BlbHandle tempHandle = NULL;

	if (!handle->subsystem->openBlob (
			statusVector, 
			&handle->handle,
			&trHandle,
			&tempHandle, 
			blobId,
			bpbLength, 
			bpb))
		{
		YBlob *blob = new YBlob (handle, tempHandle);
		*(isc_blob_handle*) blbHandle = (isc_blob_handle) blobHandles.allocateHandle (blob);
		}
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::blobInfo(ISC_STATUS* userStatus, BlbHandle *blbHandle, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	trace ("blobInfo");
	StatusVector statusVector (userStatus, traceFlags);
	YBlob *blob = getBlob (blbHandle);

	if (!blob)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	blob->subsystem->subsystem->blobInfo (statusVector,
									&blob->handle,
									itemsLength,
									items,
									bufferLength,
									buffer);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::putSegment(ISC_STATUS* userStatus, BlbHandle *blbHandle, int segmentLength, UCHAR* segment)
{
	trace ("putSegment");
	StatusVector statusVector (userStatus, traceFlags);
	YBlob *blob = getBlob (blbHandle);

	if (!blob)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	blob->subsystem->subsystem->putSegment (statusVector,
									&blob->handle,
									segmentLength,
									segment);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::getSegment (ISC_STATUS* userStatus, BlbHandle *blbHandle, int* segmentLength, int bufferLength, UCHAR* buffer)
{
	trace ("getSegment");
	StatusVector statusVector (userStatus, traceFlags);
	YBlob *blob = getBlob (blbHandle);

	if (!blob)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	ISC_STATUS ret = blob->subsystem->subsystem->getSegment (statusVector,
									&blob->handle,
									segmentLength,
									bufferLength,
									buffer);
	
	// Special case non-terminating status returns 
	
	if (ret == 0 || ret == isc_segstr_eof || ret == isc_segment)
		return ret;

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::closeBlob (ISC_STATUS* userStatus, BlbHandle *blbHandle)
{
	trace ("closeBlob");
	StatusVector statusVector (userStatus, traceFlags);
	YBlob *blob = getBlob (blbHandle);

	if (!blob)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	if (!blob->closeBlob (statusVector))
		{
		delete blob;
		releaseBlobHandle (blbHandle);
		}
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::cancelBlob(ISC_STATUS* userStatus, BlbHandle *blbHandle)
{
	trace ("cancelBlob");
	StatusVector statusVector (userStatus, traceFlags);
	YBlob *blob = getBlob (blbHandle);

	if (!blob)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	if (!blob->cancelBlob (statusVector))
		{
		delete blob;
		releaseBlobHandle (blbHandle);
		}
	
	return statusVector.getReturn();
}



ISC_STATUS Dispatch::putSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
							  SLONG* arrayId, int sdlLength, const UCHAR* sdl, int paramLength, const UCHAR* param, 
							  SLONG sliceLength, const UCHAR* slice)
{
	trace ("putSlice");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (handle);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	handle->subsystem->putSlice(statusVector,
									&handle->handle,
									&trHandle,
									arrayId, 
									sdlLength, 
									sdl, 
									paramLength, 
									param, 
									sliceLength, 
									slice);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::getSlice(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
							  SLONG* arrayId, 
							  int sdlLength, const UCHAR *sdl, 
							  int paramLength, const UCHAR *param, 
							  SLONG sliceLength, UCHAR *slice, SLONG *returnLength)
{
	trace ("getSlice");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (handle);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	handle->subsystem->getSlice(statusVector,
									&handle->handle,
									&trHandle,
									arrayId, 
									sdlLength, 
									sdl, 
									paramLength, 
									param, 
									sliceLength, 
									slice,
									returnLength);
	
	return statusVector.getReturn();
}



ISC_STATUS Dispatch::cancelEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId)
{
	trace ("cancelEvents");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);
		
	handle->subsystem->cancelEvents(
			statusVector, 
			&handle->handle, 
			eventId);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::queEvents(ISC_STATUS* userStatus, DbHandle *dbHandle, SLONG* eventId, int eventsLength, const UCHAR* events, FPTR_VOID ast,void* astArg)
{
	trace ("queEvents");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);
		
	handle->subsystem->queEvents(
			statusVector, 
			&handle->handle, 
			eventId,
			eventsLength,
			events,
			ast,
			astArg);

	return statusVector.getReturn();
}



ISC_STATUS Dispatch::dsqlAllocateStatement(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	trace ("dsqlAllocateStatement");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	if (*(isc_stmt_handle*) dsqlHandle)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	DsqlHandle tempHandle = NULL;

	if (!handle->subsystem->dsqlAllocateStatement (
			statusVector, 
			&handle->handle, 
			&tempHandle))
		{
		YStatement *statement = new YStatement (handle, tempHandle);
		*(isc_stmt_handle*) dsqlHandle = (isc_stmt_handle) statementHandles.allocateHandle (statement);
		//*dsqlHandle = (DsqlHandle*) statement;
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlAllocateStatement2(ISC_STATUS* userStatus, DbHandle *dbHandle, DsqlHandle *dsqlHandle)
{
	trace ("dsqlAllocateStatement");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	if (*(isc_stmt_handle*) dsqlHandle)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	DsqlHandle tempHandle = NULL;

	if (!handle->subsystem->dsqlAllocateStatement (
			statusVector, 
			&handle->handle, 
			&tempHandle))
		{
		YStatement *statement = new YStatement (handle, tempHandle);
		statement->userPtr = dsqlHandle;
		*(isc_stmt_handle*) dsqlHandle = (isc_stmt_handle) statementHandles.allocateHandle (statement);
		//*dsqlHandle = (DsqlHandle*) statement;
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlSqlInfo(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, 
								 int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	trace ("dsqlSqlInfo");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;

	statement->subsystem->subsystem->dsqlSqlInfo (
				statusVector,
				&statement->handle,
				itemsLength,
				items,
				bufferLength, buffer);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlSetCursor(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, TEXT* cursorName, int cursorType)
{
	trace ("dsqlSetCursor");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	statement->subsystem->subsystem->dsqlSetCursor (
				statusVector,
				&statement->handle,
				cursorName,
				cursorType);
				
	return statusVector.getReturn();
}

ISC_STATUS Dispatch::dsqlPrepare(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int sqlLength, const TEXT *sql, int dialect, XSQLDA *sqlda)
{
	trace ("dsqlPrepare");
	traceSql (sqlLength, sql);

	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);
	YStatement *statement = getStatement (dsqlHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	TraHandle thandle = transaction->getDbHandle (subsystem);
	
	if (!thandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	YSQLDA ysqlda (dialect, sqlda);
	ysqlda.allocBuffer();

	statement->subsystem->subsystem->dsqlPrepare (
				statusVector,
				&thandle,
				&statement->handle,
				sqlLength,
				sql,
				dialect,
				sizeof (prepareInfo),
				prepareInfo,
				ysqlda.bufferLength,
				ysqlda.buffer);

	if (statusVector.success())
		ysqlda.populateSqlda(ysqlda.buffer, ysqlda.bufferLength);

	return statusVector.getReturn();
}

ISC_STATUS Dispatch::dsqlPrepare(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, 
								 int sqlLength, const TEXT *sql, int dialect, 
								 int itemLength, const UCHAR *items, 
								 int bufferLength, UCHAR *buffer)
{
	trace ("dsqlPrepare");
	traceSql (sqlLength, sql);
	StatusVector statusVector (userStatus, traceFlags);
	YTransaction *transaction = getTransaction (traHandle);
	YStatement *statement = getStatement (dsqlHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	TraHandle thandle = transaction->getDbHandle (subsystem);
	
	if (!thandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	statement->subsystem->subsystem->dsqlPrepare (
				statusVector,
				&thandle,
				&statement->handle,
				sqlLength,
				sql,
				dialect,
				itemLength,
				items,
				bufferLength,
				buffer);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlDescribe(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	trace ("dsqlDescribe");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	YSQLDA ysqlda (dialect, sqlda);
	ysqlda.allocBuffer();

	statement->subsystem->subsystem->dsqlSqlInfo (
				statusVector,
				&statement->handle,
				sizeof(selectInfo),
				selectInfo,
				ysqlda.bufferLength,
				ysqlda.buffer);

	if (statusVector.success())
		ysqlda.populateSqlda(ysqlda.buffer, ysqlda.bufferLength);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlDescribeBind(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA * sqlda)
{
	trace ("dsqlDescribeBind");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	YSQLDA ysqlda (dialect, sqlda);
	ysqlda.allocBuffer();

	statement->subsystem->subsystem->dsqlSqlInfo (
				statusVector,
				&statement->handle,
				sizeof(bindInfo),
				bindInfo,
				ysqlda.bufferLength,
				ysqlda.buffer);

	if (statusVector.success())
		ysqlda.populateSqlda(ysqlda.buffer, ysqlda.bufferLength);

	return statusVector.getReturn();
}

/***
ISC_STATUS Dispatch::dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	trace ("dsqlInsert");
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Dispatch::dsqlInsert(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int blrLength, const UCHAR* blr, int msgType, int msgLength, const UCHAR* msg)
{
	trace ("dsqlInsert");
	return entrypointUnavailable (userStatus);
}
***/

ISC_STATUS Dispatch::dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int dialect, XSQLDA *sqlda)
{
	trace ("dsqlFetch");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	YSQLDA *outYSqlda = &statement->outYSqlda;
	outYSqlda->setSqlda(dialect, sqlda);
	
	ISC_STATUS ret = statement->subsystem->subsystem->dsqlFetch (
				statusVector,
				&statement->handle,
				outYSqlda->blrLength,
				outYSqlda->buffer,
				0,
				outYSqlda->msgLength,
				outYSqlda->msg);
	
	if (ret && userStatus)
		return ret;
	
	if (statusVector.success())
		outYSqlda->copyFromMessage();

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlFetch(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, 
							   int blrLength, const UCHAR* blr, int msgType, int msgLength, UCHAR* msg)
{
	trace ("dsqlFetch");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	
	ISC_STATUS ret = statement->subsystem->subsystem->dsqlFetch (
				statusVector,
				&statement->handle,
				blrLength,
				blr,
				msgType,
				msgLength,
				msg);
	
	if (ret && userStatus)
		return ret;
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
								 int sqlLength, const char *sql, int dialect,
								 XSQLDA *inSqlda, XSQLDA *outSqlda)
{
	trace ("dsqlExecuteImmediate");
	traceSql (sqlLength, sql);
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *subsystem = getDatabase (dbHandle);

	if (!subsystem)
		{
		try
			{
			SpecialSql hack (sqlLength, sql, dialect);
			Element *element = hack.parse();
			const char *fileName = element->getAttributeValue("filename", NULL);
			
			if (fileName)
				{
				PBGen dpb;
				int dpbLength = hack.genDPB(&dpb);

				if (!createDatabase(statusVector, fileName, fileName, dbHandle, 
							dpbLength, dpb.buffer, 0, NULL, NULL))
					if (element->children)
						alterDatabase (statusVector, dbHandle, &hack);

				return statusVector.getReturn();
				}
			}
		catch (OSRIException& exception)
			{
			return exception.copy (statusVector);
			}

		return statusVector.postAndReturn (isc_bad_db_handle);
		}

	YTransaction *transaction = getTransaction (traHandle);
	TraHandle thandle = (transaction) ? transaction->getDbHandle (subsystem) : NULL;
	
	if (transaction && !thandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle priorHandle = thandle;
	DbHandle dhandle = subsystem->handle;
	YSQLDA inYSqlda (dialect, inSqlda);
	inYSqlda.genBlr();
	inYSqlda.copyToMessage();
	YSQLDA outYSqlda (dialect, outSqlda);
	outYSqlda.genBlr();
	outYSqlda.allocateMessage();

	ISC_STATUS ret = subsystem->subsystem->dsqlExecuteImmediate2 (
				statusVector,
				&dhandle,
				&thandle,
				sqlLength,
				sql,
				dialect,
				inYSqlda.blrLength,
				inYSqlda.buffer,
				0,		// input message type???
				inYSqlda.msgLength,
				inYSqlda.msg,
				outYSqlda.blrLength,
				outYSqlda.buffer,
				1,		// output message type???
				outYSqlda.msgLength,
				outYSqlda.msg);
	
	if (!ret)
		{
		if (thandle != priorHandle)
			{
			if (transaction)
				{
				releaseTransaction (traHandle);
				delete transaction;
				transaction = NULL;
				}
			if (thandle)
				{
				transaction = new YTransaction (1);
				transaction->setTransactionHandle (subsystem, thandle);
				SET_HANDLE (isc_tr_handle, traHandle, transactionHandles.allocateHandle (transaction));
				}
			}
		if (!dhandle && subsystem)
			{
			*dbHandle = NULL;
			delete subsystem;
			}
		}

	if (statusVector.success())
		outYSqlda.copyFromMessage();

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlExecute (ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle,
								 int inBlrLength, const UCHAR *inBlr, 
								 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
								 int outBlrLength, const UCHAR *outBlr, 
								 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	trace ("dsqlExecute");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	TraHandle thandle = transaction->getDbHandle (subsystem);
	
	if (!thandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle priorTHandle = thandle;
	DsqlHandle shandle = statement->handle;

	ISC_STATUS ret = statement->subsystem->subsystem->dsqlExecute (
				statusVector,
				&thandle,
				&shandle,
				inBlrLength,
				inBlr,
				inMsgType,
				inMsgLength,
				inMsg,
				outBlrLength,
				outBlr,
				outMsgType,	
				outMsgLength,
				outMsg);
	
	if (!ret)
		{
		if (thandle != priorTHandle)
			{
			if (transaction)
				{
				releaseTransaction (traHandle);
				delete transaction;
				transaction = NULL;
				}
			if (thandle)
				{
				transaction = new YTransaction (1);
				transaction->setTransactionHandle (subsystem, thandle);
				SET_HANDLE (isc_tr_handle, traHandle, transactionHandles.allocateHandle (transaction));
				}
			}
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlExecute (ISC_STATUS* userStatus, 
								  TraHandle *traHandle, 
								  DsqlHandle *dsqlHandle, 
								  int dialect, 
								  XSQLDA *inSqlda, 
								  XSQLDA *outSqlda)
{
	trace ("dsqlExecute");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);
	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_stmt_handle);

	SubsysHandle *subsystem = statement->subsystem;
	TraHandle thandle = transaction->getDbHandle (subsystem);
	
	if (!thandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle priorTHandle = thandle;
	DsqlHandle shandle = statement->handle;
	YSQLDA inYSqlda (dialect, inSqlda);
	inYSqlda.genBlr();
	inYSqlda.copyToMessage();
	YSQLDA outYSqlda (dialect, outSqlda);
	outYSqlda.genBlr();
	outYSqlda.allocateMessage();

	ISC_STATUS ret = statement->subsystem->subsystem->dsqlExecute (
				statusVector,
				&thandle,
				&shandle,
				inYSqlda.blrLength,
				inYSqlda.buffer,
				0,		// input message type???
				inYSqlda.msgLength,
				inYSqlda.msg,
				outYSqlda.blrLength,
				outYSqlda.buffer,
				1,		// output message type???
				outYSqlda.msgLength,
				outYSqlda.msg);
	
	
	if (!ret)
		{
		if (thandle != priorTHandle)
			{
			if (transaction)
				{
				releaseTransaction (traHandle);
				delete transaction;
				transaction = NULL;
				}
			if (thandle)
				{
				transaction = new YTransaction (1);
				transaction->setTransactionHandle (subsystem, thandle);
				SET_HANDLE (isc_tr_handle, traHandle, transactionHandles.allocateHandle (transaction));
				}
			}
		if (!shandle && statement)
			{
			delete statement;
			releaseStatementHandle (dsqlHandle);
			SET_HANDLE (isc_tr_handle, traHandle, 0);
			}
		}

	if (statusVector.success())
		outYSqlda.copyFromMessage();

	return statusVector.getReturn();
}


//ISC_STATUS Dispatch::dsqlExecute2(ISC_STATUS* userStatus, TraHandle *traHandle, DsqlHandle *dsqlHandle, int, UCHAR*,int, int, UCHAR*, int, UCHAR*, int, int,UCHAR*)


ISC_STATUS Dispatch::dsqlExecuteImmediate2(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, const UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
										 int outBlrLength, const UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	trace ("dsqlExecuteImmediate2");
	traceSql (sqlLength, sql);
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *subsystem = getDatabase (dbHandle);

	if (!subsystem)
		{
		try
			{
			SpecialSql hack (sqlLength, sql, dialect);
			Element *element = hack.parse();
			const char *fileName = element->getAttributeValue("filename", NULL);
			
			if (fileName)
				{
				PBGen dpb;
				int dpbLength = hack.genDPB(&dpb);
				dbHandle = NULL;
				if (!createDatabase(statusVector, fileName, fileName, dbHandle, 
							dpbLength, dpb.buffer,0, NULL, NULL))
					if (element->children)
						alterDatabase (statusVector, dbHandle, &hack);

				return statusVector.getReturn();
				}
			}
		catch (OSRIException& exception)
			{
			return exception.copy (statusVector);
			}

		return statusVector.postAndReturn (isc_bad_db_handle);
		}

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle thandle = transaction->getDbHandle (subsystem);
	
	if (!thandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle priorTHandle = thandle;
	DbHandle dhandle = subsystem->handle;

	ISC_STATUS ret = subsystem->subsystem->dsqlExecuteImmediate2 (
				statusVector,
				&dhandle,
				&thandle,
				sqlLength, sql, dialect,
				inBlrLength, inBlr,
				inMsgType, inMsgLength, inMsg,
				outBlrLength, outBlr,
				outMsgType, outMsgLength, outMsg);
	
	if (!ret)
		{
		if (thandle != priorTHandle)
			{
			if (transaction)
				{
				releaseTransaction (traHandle);
				delete transaction;
				transaction = NULL;
				}
			if (thandle)
				{
				transaction = new YTransaction (1);
				transaction->setTransactionHandle (subsystem, thandle);
				SET_HANDLE (isc_tr_handle, traHandle, transactionHandles.allocateHandle (transaction));
				}
			}
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::dsqlExecuteImmediate3(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int inBlrLength, const UCHAR *inBlr, 
										 int inMsgType, int inMsgLength, const UCHAR *inMsg, 
										 int outBlrLength, const UCHAR *outBlr, 
										 int outMsgType, int outMsgLength, UCHAR *outMsg)
{
	trace ("dsqlExecuteImmediate3");
	
	return dsqlExecuteImmediate2 (
				userStatus,
				dbHandle,
				traHandle,
				sqlLength, sql, dialect,
				inBlrLength, inBlr,
				inMsgType, inMsgLength, inMsg,
				outBlrLength, outBlr,
				outMsgType, outMsgLength, outMsg);
}


ISC_STATUS Dispatch::dsqlExecuteImmediate (ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, 
										 int sqlLength, const char* sql, int dialect, 
										 int blrLength, const UCHAR *blr, 
										 int msgType, int msgLength, UCHAR* msg)
{
	trace ("dsqlExecuteImmediate");

	return dsqlExecuteImmediate2 (
				userStatus,
				dbHandle,
				traHandle,
				sqlLength, sql, dialect,
				blrLength, blr,					// inBlrLength, inBlr,
				msgType, msgLength, msg,	// inMsgType, inMsgLength, inMsg,
				0, NULL,						//outBlrLength, outBlr,
				0, 0, NULL);					//outMsgType, outMsgLength, outMsg);
}


ISC_STATUS Dispatch::dsqlFreeStatement(ISC_STATUS* userStatus, DsqlHandle *dsqlHandle, int option)
{
	trace ("dsqlFreeStatement");
	StatusVector statusVector (userStatus, traceFlags);
	YStatement *statement = getStatement (dsqlHandle);

	if (!statement)
		return statusVector.postAndReturn (isc_bad_req_handle);

	// call below zeroes what dsqlHandle pointed to
	// hence we must release a copy of real handle
	
	isc_stmt_handle handle = *(isc_stmt_handle*) dsqlHandle;
	
	if (!statement->releaseStatement(statusVector, option, false) && (option & DSQL_drop))
		{
		delete statement;
		statementHandles.releaseHandle(handle);
		*(isc_stmt_handle*) dsqlHandle = 0;
		}

	return statusVector.getReturn();
}



//ISC_STATUS Dispatch::cancelOperation(ISC_STATUS* userStatus, DbHandle *dbHandle, int);
ISC_STATUS Dispatch::serviceQuery(ISC_STATUS *userStatus, 
								SvcHandle *svcHandle, 
								int inItemLength, 
								const UCHAR* inItem, 
								int outItemLength, 
								const UCHAR* outItem, 
								int bufferLength, 
								UCHAR *buffer)
{
	trace ("serviceQuery");
	StatusVector statusVector (userStatus, traceFlags);
	YService *service = getService (svcHandle);

	if (!service)
		return statusVector.postAndReturn (isc_bad_svc_handle);

	service->subsystem->serviceQuery (
			statusVector, 
			&service->handle, 
			inItemLength, inItem,
			outItemLength, outItem,
			bufferLength, buffer);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::serviceDetach(ISC_STATUS *userStatus, SvcHandle *svcHandle)
{
	trace ("serviceDetach");
	StatusVector statusVector (userStatus, traceFlags);
	YService *service = getService (svcHandle);

	if (!service)
		return statusVector.postAndReturn (isc_bad_svc_handle);

	if (!service->subsystem->serviceDetach (statusVector, &service->handle))
		{
		int slot = getHandleSlot (svcHandle);
		serviceHandles.releaseHandle (slot);
		*((isc_svc_handle*) svcHandle) = 0;
		delete service;
		}

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::serviceAttach(ISC_STATUS *userStatus, 
								  const TEXT *service, 
								  SvcHandle *dbHandle, 
								  int spbLength, 
								  const UCHAR *spb,
								  ConfObject* servicesConfiguration,
								  ConfObject* providerConfiguration)
{
	if (!initialized)
		initialize();
		
	if (traceFlags & traceSQL)
		printTrace ("serviceAttach %s", service);
	else
		trace ("serviceAttach %s", service);
	
	StatusVector localVector (userStatus, traceFlags);
	StatusVector *statusVector = &localVector;
	ISC_STATUS localStatus[ISC_STATUS_LENGTH];

	if (*((isc_svc_handle*) dbHandle))
		return statusVector->postAndReturn (isc_bad_db_handle);

	if (spbLength > 0 && !spb)
		return statusVector->postAndReturn (isc_bad_spb_form);

	bool hasProvider = false;
	bool haveError = false;
	ConfObject *confObject;
	
	try
		{
		confObject = Configuration::findObject ("services", "generic");
		
		if (!confObject)
			{
			statusVector->post(isc_bad_db_format, isc_arg_string, service, isc_arg_end);
			return statusVector->getReturn();
			}
		}
	catch (AdminException& exception)
		{
		exception;
		statusVector->post(isc_bad_db_format, isc_arg_string, service, isc_arg_end);
		return statusVector->getReturn();
		}
	
	for (int n = 0;; ++n)
		{
		JString providerName = confObject->getValue (n, "provider");
		
		if (providerName.IsEmpty())
			break;
			
		hasProvider = true;
		Provider *provider = getProvider (providerName);
		
		if (provider->subsystems)
			for (int subsys = 0;; ++subsys)
				{
				Subsystem *subsystem = provider->subsystems [subsys];
				
				if (!subsystem)
					break;
					
				SvcHandle tempHandle = NULL;
				
				if (subsystem->serviceAttach (*statusVector, service, &tempHandle, spbLength, spb, 
											  confObject, provider->configuration))
					{
					if (!haveError)
						memcpy (localStatus, statusVector->statusVector, sizeof(localStatus));

					haveError = true;
					}
				else
					{
					YService *handle = new YService (subsystem, tempHandle);
					*((isc_svc_handle*) dbHandle) = (isc_svc_handle) serviceHandles.allocateHandle (handle);
					return statusVector->getCode();
					}
				}
		}

	if (haveError)
		{
		memcpy (statusVector->statusVector, localStatus, sizeof(localStatus));
		return statusVector->getReturn();
		}
		
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Dispatch::serviceStart(ISC_STATUS* userStatus,
								 SvcHandle *svcHandle,
								 int spbLength, 
								 const UCHAR * spb)
{
	trace ("serviceStart");
	StatusVector statusVector (userStatus, traceFlags);
	YService *service = getService (svcHandle);

	if (!service)
		return statusVector.postAndReturn (isc_bad_svc_handle);

	service->subsystem->serviceStart (
			statusVector, 
			&service->handle, 
			spbLength, spb);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::transactRequest(ISC_STATUS* userStatus, 
								   DbHandle *dbHandle, 
								   TraHandle *traHandle, 
								   int blrLength, 
								   const UCHAR* blr,
								   int inMsgLength, 
								   const UCHAR* inMsg, 
								   int outMsgLength, 
								   UCHAR* outMsg)
{
	trace ("transactRequest");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (handle);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	handle->subsystem->transactRequest (
			statusVector, 
			&handle->handle,
			&trHandle,
			blrLength,
			blr,
			inMsgLength,
			inMsg,
			outMsgLength,
			outMsg);
	
	return statusVector.getReturn();
}


ISC_STATUS Dispatch::executeDDL(ISC_STATUS* userStatus, DbHandle *dbHandle, TraHandle *traHandle, int ddlLength, const UCHAR* ddl)
{
	trace ("executeDDL");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	YTransaction *transaction = getTransaction (traHandle);

	if (!transaction)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	TraHandle trHandle = transaction->getDbHandle (handle);

	if (!trHandle)
		return statusVector.postAndReturn (isc_bad_trans_handle);

	handle->subsystem->executeDDL (
			statusVector, 
			&handle->handle,
			&trHandle,
			ddlLength,
			ddl);
	
	return statusVector.getReturn();
}

/***
int Dispatch::enableSubsystem (TEXT* subSystem)
{
	trace ("enableSubsystem");
	return false;
}


int Dispatch::disableSubsystem (TEXT* subSystem)
{
	trace ("disableSubsystem");
	return false;
}
***/

/***
ISC_STATUS Dispatch::databaseCleanup (ISC_STATUS* userStatus, 
									 DbHandle *dbHandle, 
									 DatabaseCleanupRoutine *routine, 
									 SLONG arg)
{
	trace ("databaseCleanup");
	return entrypointUnavailable (userStatus);
}


ISC_STATUS Dispatch::transactionCleanup (ISC_STATUS* userStatus, 
									   TraHandle *traHandle, 
									   TransactionCleanupRoutine *routine, 
									   SLONG arg)
{
	trace ("transactionCleanup");
	return entrypointUnavailable (userStatus);
}
***/

ISC_STATUS Dispatch::seekBlob (ISC_STATUS* userStatus, 
							 BlbHandle *blbHandle,
							 int mode,
							 SLONG offset,
							 SLONG *result)
{
	trace ("seekBlob");
	StatusVector statusVector (userStatus, traceFlags);
	YBlob *blob = getBlob (blbHandle);

	if (!blob)
		return statusVector.postAndReturn (isc_bad_segstr_handle);

	blob->subsystem->subsystem->seekBlob (statusVector,
									&blob->handle,
									mode,
									offset,
									result);
	
	return statusVector.getReturn();
}




ISC_STATUS Dispatch::updateAccountInfo(ISC_STATUS *userStatus, DbHandle *dbHandle, int apbLength, const UCHAR *apb)
{
	trace ("updateAccountInfo");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	handle->subsystem->updateAccountInfo (statusVector, &handle->handle, apbLength, apb);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::postError(ISC_STATUS *statusVector, ISC_STATUS *userVector)
{
	trace ("postError");
	if (userVector != statusVector)
		{
		isc_print_status (statusVector);
		exit (statusVector [1]);
		}

	return statusVector [1];
}

/***
ISC_STATUS Dispatch::registerCleanupHandler(ISC_STATUS *userStatus, DbHandle *handle, DatabaseCleanupRoutine *routine, void *arg)
{
	trace ("registerCleanupHandler");
	return entrypointUnavailable (userStatus);
}
***/
int Dispatch::rewriteDpb(int dpbLength, UCHAR **dpbPtr)
{
	UCHAR *p = *dpbPtr, *end = p + dpbLength;
	bool haveUserName = false;
	bool havePassword = false;
	
	if (dpbLength)
		{
		if (*p++ != isc_dpb_version1)
			return dpbLength;
			
		while (p < end)
			{
			switch (*p++)
				{
				case isc_dpb_sys_user_name:
				case isc_dpb_user_name:
					haveUserName = true;
					break;

				case isc_dpb_password:
				case isc_dpb_password_enc:
					havePassword = true;
					break;
				}
				
			int l = *p++;
			p += l;
			}
		}

	if (haveUserName && havePassword)
		return dpbLength;

	const char *userName = (haveUserName) ? NULL : getenv ("ISC_USER");
	const char *password = (havePassword) ? NULL : getenv ("ISC_PASSWORD");
	int newDpbLength = dpbLength;
	int nameLength = 0;
	int passwordLength = 0;

	if (!dpbLength)
		++newDpbLength;

	if (userName)
		{
		nameLength = strlen (userName);
		newDpbLength += nameLength + 2;
		}

	if (password)
		{
		passwordLength = strlen (password);
		newDpbLength += passwordLength + 2;
		}

	UCHAR *dpb = new UCHAR [newDpbLength];

	if (dpbLength)
		{
		memcpy (dpb, *dpbPtr, dpbLength);
		p = dpb + dpbLength;
		}
	else
		{
		p = dpb;
		*p++ = isc_dpb_version1;
		}

	if (userName)
		stuff (&p, isc_dpb_user_name, nameLength, userName);
		
	if (password)
		stuff (&p, isc_dpb_password, passwordLength, password);

	*dpbPtr = dpb;
	
	return newDpbLength;		
}

void Dispatch::stuff(UCHAR **ptr, UCHAR what, int length, const char *data)
{
	UCHAR *p = *ptr;
	*p++ = what;
	*p++ = length;
	memcpy (p, data, length);

	*ptr = p + length;
}

Provider* Dispatch::getProvider(JString providerName)
{
	Provider *provider;
	
	Sync sync(&syncProvider, "getProvider");
	sync.lock(Shared);
	
	for (provider = providers; provider; provider = provider->next)
		if (provider->name == providerName)
			return provider;
	
	sync.unlock();
	
	sync.lock(Exclusive);
	for (provider = providers; provider; provider = provider->next)
		if (provider->name == providerName)
			return provider;
	
	ConfObject *confObject = Configuration::findObject ("provider", providerName);
	provider = new Provider (providerName, confObject, NULL);
	provider->next = providers;
	providers = provider;
	
	return provider;
}

ISC_STATUS Dispatch::postError(ISC_STATUS *userStatus, ISC_STATUS code)
{
	StatusVector statusVector (userStatus, traceFlags);
	statusVector.postCode (code);

	return statusVector.getReturn();
}

void Dispatch::trace(const char* method, ...)
{
	if (traceFlags & traceCalls)
		{
		va_list		args;
		va_start	(args, method);
		printf ("Dispatch: ");
		vprintf (method, args);
		va_end(args);
		printf ("\n");
		}
}

void Dispatch::printTrace(const char* method, ...)
{
	va_list		args;
	va_start	(args, method);
	printf ("Dispatch: ");
	vprintf (method, args);
	va_end(args);
	printf ("\n");
}

void Dispatch::traceSql(int length, const char* sql)
{
	if (traceFlags & traceSQL)
		if (length)
			printf ("sql: %.*s\n", length, sql);
		else
			printf ("sql: %s\n", sql);
}

YTransaction* Dispatch::getTransaction(TraHandle* traHandle)
{
	int slot = (IPTR) *((isc_tr_handle*) traHandle);
	
	return (YTransaction*) transactionHandles.getObject(slot);
}

YStatement* Dispatch::getStatement(DsqlHandle* dsqlHandle)
{
	int slot = getHandleSlot (dsqlHandle);
	
	return (YStatement*) statementHandles.getObject(slot);
}

YBlob* Dispatch::getBlob(BlbHandle* blobHandle)
{
	int slot = getHandleSlot (blobHandle);
	
	return (YBlob*) blobHandles.getObject(slot);
}

SubsysHandle* Dispatch::getDatabase(DbHandle* dbHandle)
{
	int slot = getHandleSlot (dbHandle);
	
	return (SubsysHandle*) databaseHandles.getObject(slot);
}

YRequest* Dispatch::getRequest(ReqHandle* reqHandle)
{
	int slot = getHandleSlot (reqHandle);
	
	return (YRequest*) requestHandles.getObject(slot);
}

int Dispatch::getHandleSlot(void* handle)
{
	if (!handle)
		return 0;
		
	return (IPTR) *((isc_db_handle*) handle);
}

void Dispatch::releaseTransaction(TraHandle* traHandle)
{
	int slot = getHandleSlot (traHandle);
	transactionHandles.releaseHandle (slot);
	*(isc_tr_handle*) traHandle = 0;
}

void Dispatch::releaseStatementHandle(DsqlHandle* handle)
{
	int slot = getHandleSlot (handle);
	statementHandles.releaseHandle (slot);
	*(isc_stmt_handle*) handle = 0;
}

void Dispatch::releaseBlobHandle(BlbHandle* blbHandle)
{
	int slot = getHandleSlot (blbHandle);
	blobHandles.releaseHandle (slot);
	*(isc_blob_handle*) blbHandle = 0;
}

int Dispatch::shutdownConnections(int type, int milliseconds)
{
	int count = 0;
	
	if (type > shutdownLevel)
		shutdownLevel = type;
	
	for (Provider *provider = providers; provider; provider = provider->next)
		count += provider->shutdownConnections (type, milliseconds);

	return 0;
}

ISC_STATUS Dispatch::cancelOperation(ISC_STATUS* userStatus, DbHandle* dbHandle, int flags)
{
	trace ("cancelOperation");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	handle->subsystem->cancelOperation (
			statusVector, 
			&handle->handle, 
			flags);

	return statusVector.getReturn();
}

void Dispatch::initialize(void)
{
	try
		{
		ConfObject *confObject = Configuration::findObject ("provider", "dispatch");
		
		if (confObject)
			traceFlags |= confObject->getValue(TraceFlags, TraceFlagsValue);
			
		initialized = true;
		}
	catch (AdminException& exception)
		{
		fprintf (stderr, "fatal error in configuration file: %s\n", exception.getText());
		}
}

ISC_STATUS Dispatch::authenticateUser(ISC_STATUS* userStatus, DbHandle* dbHandle, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	trace ("authenticateUser");
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	handle->subsystem->authenticateUser (statusVector, &handle->handle, 
								 dpbLength, dpb,
								 itemsLength, items, 
								 bufferLength, buffer);

	return statusVector.getReturn();
}


ISC_STATUS Dispatch::alterDatabase (ISC_STATUS* userStatus, 
									DbHandle *dbHandle, 
									SpecialSql *hack)
{
	StatusVector statusVector (userStatus, traceFlags);
	SubsysHandle *handle = getDatabase (dbHandle);

	TransactionElement	trBlock;
	trBlock.dbHandle = dbHandle;
	trBlock.tpb = NULL;
	trBlock.tpbLength = 0;


	if (!handle)
		return statusVector.postAndReturn (isc_bad_db_handle);

	JString command = hack->genAlterStatement();
	TraHandle dummy = 0;
	TraHandle *traHandle = &dummy;

	if (startMultiple(statusVector, traHandle, 1, &trBlock))
		return statusVector.getReturn();
		
	if (dsqlExecuteImmediate (statusVector, dbHandle, traHandle, 
								 command.length(), (const char *)command, 0,
								 NULL, NULL)) 
		{
		ISC_STATUS rollbackStatus [20];
		rollbackTransaction (rollbackStatus, traHandle);
		return statusVector.getReturn();
		}

	return commitTransaction (statusVector, traHandle);

}

YService* Dispatch::getService(SvcHandle* svcHandle)
{
	int slot = getHandleSlot (svcHandle);
	
	return (YService*) serviceHandles.getObject(slot);
}

ISC_STATUS Dispatch::setConfigFilename(ISC_STATUS *userStatus, const char* configFilename)
{
	StatusVector statusVector (userStatus, traceFlags);

	try
		{
		Configuration::setConfigFile(configFilename);
		}
	catch (AdminException& exception)
		{
		exception;
		}

	return statusVector.getReturn();
}

ISC_STATUS Dispatch::setConfigText(ISC_STATUS *userStatus, const char* configText)
{
	StatusVector statusVector (userStatus, traceFlags);

	try
		{
		Configuration::setConfigText(configText);
		}
	catch (AdminException& exception)
		{
		exception;
		}

	return statusVector.getReturn();
}

ISC_STATUS Dispatch::engineInfo(ISC_STATUS* userStatus, const TEXT* engineName,
								int itemsLength, const UCHAR* items, 
								int bufferLength, UCHAR* buffer)
{
	if (!initialized)
		initialize();

	trace ("engineInfo");
	StatusVector statusVector (userStatus, traceFlags);

	bool haveError = false;
	
	ConfObject *confObject = Configuration::findObject ("engines", "generic");
	
	if (!confObject)
		{
		statusVector.post(isc_bad_svc_handle, isc_arg_string, "", isc_arg_end);
		return statusVector.getReturn();
		}
		
	for (int n = 0;; ++n)
		{
		JString providerName = confObject->getValue (n, "provider");
		if (providerName.IsEmpty())
			break;
		Provider *provider = getProvider (providerName);
		if (provider->subsystems)
			for (int subsys = 0;; ++subsys)
				{
				Subsystem *subsystem = provider->subsystems [subsys];
				if (!subsystem)
					break;

				if (subsystem->engineInfo(statusVector, engineName, itemsLength, items, bufferLength, buffer))
					haveError = true;
				else
					return statusVector.getReturn();
				}
		}

	if (haveError)
		return statusVector.getCode();

	return entrypointUnavailable (userStatus);
}
