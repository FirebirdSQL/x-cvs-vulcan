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


#include <string.h>
#include "firebird.h"
#include "common.h"
#include "Services.h"
#include "iberror.h"
#include "svc_proto.h"
#include "ConfObject.h"
#include "OSRIException.h"
#include "Service.h"
#include "ibase.h"

static Services provider;
static Subsystem* subsystems [] = { &provider, NULL };

extern "C" Subsystem **getSubsystems() 
{ 
	return subsystems; 
};

Services::Services(void)
{
}

Services::~Services(void)
{
}

ISC_STATUS Services::serviceAttach(ISC_STATUS *userStatus, const TEXT* service, SvcHandle* svcHandle, 
								  int spbLength, const UCHAR* spb, 
								  ConfObject* servicesConfiguration, ConfObject* providerConfiguration)
{
	if (servicesConfiguration)
		servicesConfiguration->setChain (providerConfiguration);

	try
		{
		*svcHandle = SVC_attach(servicesConfiguration, service, spbLength, spb);
		}
	catch (OSRIException& exception)
		{
		return error(&exception, userStatus);
		}

	return returnSuccess(userStatus);
}

ISC_STATUS Services::returnSuccess(ISC_STATUS *userStatus)
{
	ISC_STATUS *p = userStatus;
	*p++ = isc_arg_gds;
	*p++ = FB_SUCCESS;
	*p = isc_arg_end;
	
	return userStatus[1];
}

ISC_STATUS Services::serviceStart(ISC_STATUS *userStatus, SvcHandle* svcHandle, int spbLength, const UCHAR* spb)
{
	Service* service = (Service*) *svcHandle;
	
	try
		{
		SVC_start(service, spbLength, spb);
	
		/***
		if (service->svc_status[1]) 
			{
			ISC_STATUS* svc_status = service->svc_status;
			ISC_STATUS* tdbb_status = threadData.getStatusVector();
	
			while (*svc_status) 
				*tdbb_status++ = *svc_status++;
				
			*tdbb_status = isc_arg_end;
			}
		***/
		
		return returnSuccess(userStatus);
		}
	catch (OSRIException& exception)
		{
		return error(&exception, userStatus);
		}
}

ISC_STATUS Services::error(OSRIException* exception, ISC_STATUS *userStatus)
{
	exception->copy (userStatus);
	
	return userStatus[1];
}

ISC_STATUS Services::serviceQuery(ISC_STATUS *userStatus, SvcHandle* svcHandle, int inItemLength, const UCHAR* inItem, int outItemLength, const UCHAR* outItem, int bufferLength, UCHAR* buffer)
{
	Service* service = (Service*) *svcHandle;
	
	try
		{
		if (service->svc_spb_version == isc_spb_version1)
			SVC_query(service, inItemLength, inItem, outItemLength, outItem, bufferLength, buffer);
		else 
			{
			/* For SVC_query2, we are going to completly dismantle user_status (since at this point it is
			* meaningless anyway).  The status vector returned by this function can hold information about
			* the call to query the service manager and/or a service thread that may have been running.
			*/
	
			SVC_query2(service, NULL, inItemLength, inItem, outItemLength, outItem, bufferLength, buffer);
	
			/* if there is a status vector from a service thread, copy it into the thread status */
			
			/***
			int len, warning;
			PARSE_STATUS(service->svc_status, len, warning);
			
			if (len) 
				{
				MOVE_FASTER(service->svc_status, threadData.threadData->tdbb_status_vector, //tdbb->tdbb_status_vector,
							sizeof(ISC_STATUS) * len);
	
				// Empty out the service status vector 
				memset(service->svc_status, 0, ISC_STATUS_LENGTH * sizeof(ISC_STATUS));
				}
	
			if (user_status[1])
				return error(&exception, user_status);
			***/
			}

		return returnSuccess(userStatus);
		}
	catch (OSRIException& exception)
		{
		return error(&exception, userStatus);
		}
}

ISC_STATUS Services::serviceDetach(ISC_STATUS *userStatus, SvcHandle* svcHandle)
{
	Service* service = (Service*) *svcHandle;
	
	try
		{
		SVC_detach(service);
		
		return returnSuccess(userStatus);
		}
	catch (OSRIException& exception)
		{
		return error(&exception, userStatus);
		}
}
