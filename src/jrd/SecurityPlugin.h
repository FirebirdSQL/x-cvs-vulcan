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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// BlrGen.h: interface for the BlrGen class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SECURITY_PLUGIN_H
#define _SECURITY_PLUGIN_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ConfObj.h"

class Connection;

class SecurityContext
{
public:
	virtual Connection*	getUserConnection() = 0;
	virtual Connection*	getNewConnection() = 0;
	virtual const char*	getDatabaseFilename() = 0;
	virtual const char* getAccount(void) = 0;
	virtual const char* getEncryptedPassword(void) = 0;
	virtual const char* getPassword(void) = 0;
};

static const int SecurityPluginVersion1 =		1;
static const int CurrentSecurityPlugVersion	=	SecurityPluginVersion1;

class SecurityPlugin
{
public:
	SecurityPlugin(SecurityPlugin *chain);
	virtual ~SecurityPlugin(void);
	
	virtual int			getAPIVersion(void);
	virtual ConfObject* getConfiguration(void);
	virtual void		authenticateUser(SecurityContext *context, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	virtual void		updateAccountInfo(SecurityContext *context, int apbLength, const UCHAR* apb);
	virtual void		close();
	
protected:
	ConfObj			configuration;
	SecurityPlugin	*chain;
};

#endif

