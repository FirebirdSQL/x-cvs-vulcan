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

// YStatement.cpp: implementation of the YStatement class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "ibase.h"
#include "common.h"
#include "dsc.h"
#include "YStatement.h"
#include "SubsysHandle.h"
#include "StatusVector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YStatement::YStatement(SubsysHandle *subsys, DsqlHandle orgHandle)
{
	subsystem = subsys;
	handle = orgHandle;
	userPtr = NULL;
	subsystem->addStatement (this);
}

YStatement::~YStatement()
{
	if (subsystem)
		subsystem->removeStatement (this);
}

ISC_STATUS YStatement::releaseStatement(StatusVector& statusVector, int option, bool force)
{
	if (!subsystem)
		return 0;

	ISC_STATUS ret = 0;

	if (handle)	
		ret = subsystem->subsystem->dsqlFreeStatement (statusVector, &handle, option);

	if (force || (!ret && (option & DSQL_drop)))
		{
		subsystem->removeStatement (this);
		subsystem = NULL;
		
		if (userPtr)
			{
			*(isc_stmt_handle*)userPtr = NULL;
			userPtr = NULL;
			}
		}

	return ret;
}
