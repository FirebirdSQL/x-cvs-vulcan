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


#include "firebird.h"
#include "Services.h"
#include "iberror.h"

static Services provider;
static Subsystem* subsystems [] = { &provider, NULL };
extern "C" Subsystem **getSubsystems() { return subsystems; };

Services::Services(void)
{
}

Services::~Services(void)
{
}
