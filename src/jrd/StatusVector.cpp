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

// StatusVector.cpp: implementation of the StatusVector class.
//
//////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "firebird.h"
#include "ibase.h"
#include "StatusVector.h"
#include "StatusPrint.h"
#include "gds_proto.h"

#define DEBUG

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

StatusVector::StatusVector(ISC_STATUS *userStatus, int flags)
{
	userVector = userStatus;
	statusVector = (userVector) ? userVector : localVector;
	statusVector[0] = isc_arg_gds;
	statusVector[1] = 0;
	statusVector[2] = isc_arg_end;
	options = flags;
}

StatusVector::~StatusVector()
{

}

StatusVector::operator ISC_STATUS*()
{
	return statusVector;
}

ISC_STATUS StatusVector::getCode()
{
	return statusVector [1];
}

ISC_STATUS StatusVector::printStatus()
{
	StatusPrint obj;
	obj.printStatus (statusVector);
	
	return statusVector [1];
}

ISC_STATUS StatusVector::getReturn()
{
	/***
	if (statusVector [1])
		printf ("StatusVector::getReturn: %d\n", statusVector [1]);
	***/

	if (statusVector [1] && !userVector)
		{
		printStatus();
		terminateProcess();
		}

	if (statusVector [1] && (options & statusReport))
		printStatus();

	return statusVector [1];
}

void StatusVector::terminateProcess()
{
	exit (statusVector [1]);
}


ISC_STATUS StatusVector::post(ISC_STATUS iscStatus, ...)
{
	va_list		args;
	va_start	(args, iscStatus);
	ISC_STATUS	*v = statusVector;

	*v++ = isc_arg_gds;
	*v++ = iscStatus;

	for (;;)
		{
		ISC_STATUS type = va_arg(args, ISC_STATUS);
		*v++ = type;
		if (type == isc_arg_end)
			break;;
		switch (type)
			{
			case isc_arg_gds:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_dos:
			case isc_arg_win32:
			case isc_arg_number:
				*v++ = va_arg (args, int);
				break;

			case isc_arg_string:
			case isc_arg_interpreted:
				*v++ = (ISC_STATUS) va_arg (args, TEXT*);
				break;

			case isc_arg_cstring:
				*v++ = va_arg (args, int);
				*v++ = (ISC_STATUS) va_arg (args, TEXT*);
				break;
			}
		}

	return statusVector [1];
}

ISC_STATUS StatusVector::postCode(ISC_STATUS code)
{
	return post (code, isc_arg_end);
}

ISC_STATUS StatusVector::postAndReturn(ISC_STATUS code)
{
	postCode (code);

	return getReturn();
}

bool StatusVector::success()
{
	return statusVector [1] == 0;
}

