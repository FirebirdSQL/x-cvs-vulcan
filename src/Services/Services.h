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

#ifndef _SERVICE_H_
#define _SERVICE_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Subsystem.h"

CLASS (OSRIException);

class Services : public Subsystem
{
public:
	Services(void);
	virtual ~Services(void);
	ISC_STATUS returnSuccess(ISC_STATUS *userStatus);
	ISC_STATUS error(OSRIException* exception, ISC_STATUS *userStatus);
	virtual ISC_STATUS serviceAttach(ISC_STATUS *userStatus, 
										const TEXT* service, 
										SvcHandle* dbHandle, 
										int spbLength, 
										const UCHAR* spb, 
										ConfObject* servicesConfiguration, 
										ConfObject* providerConfiguration);
	virtual ISC_STATUS serviceStart(ISC_STATUS *userStatus, SvcHandle* svcHandle, int spbLength, const UCHAR* spb);
	virtual ISC_STATUS serviceQuery(ISC_STATUS *userStatus, SvcHandle* svcHandle, int inItemLength, const UCHAR* inItem, int outItemLength, const UCHAR* outItem, int bufferLength, UCHAR* buffer);
	virtual ISC_STATUS serviceDetach(ISC_STATUS *userStatus, SvcHandle* svcHandle);
};

#endif

