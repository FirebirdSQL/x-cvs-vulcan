/*
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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */
 
#include "firebird.h"
#include "common.h"
#include "SecurityPlugin.h"

SecurityPlugin::SecurityPlugin(SecurityPlugin *securityChain)
{
	if (chain = securityChain)
		configuration = chain->getConfiguration();
}

SecurityPlugin::~SecurityPlugin(void)
{
	if (chain)
		delete chain;
}


ConfObject* SecurityPlugin::getConfiguration(void)
{
	return configuration;
}

void SecurityPlugin::updateAccountInfo(SecurityContext *context, int apbLength, const UCHAR* apb)
{
	return chain->updateAccountInfo(context, apbLength, apb);
}

void SecurityPlugin::authenticateUser(SecurityContext *context, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
}

int SecurityPlugin::getAPIVersion(void)
{
	if (!chain)
		return CurrentSecurityPlugVersion;
	
	int version = chain->getAPIVersion();
	
	return (version < CurrentSecurityPlugVersion) ? version : CurrentSecurityPlugVersion;
}
