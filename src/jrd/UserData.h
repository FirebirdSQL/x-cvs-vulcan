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

#ifndef _USER_DATA_H
#define _USER_DATA_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RefObject.h"
#include "JString.h"

class UserData :
	public RefObject
{
public:
	UserData(void);
	virtual ~UserData(void);

	JString		userName;
	JString		firstName;
	JString		middleName;
	JString		lastName;
	JString		fullName;
	JString		systemName;
	JString		groupName;
	JString		password;
	JString		encryptedPassword;
	JString		roleName;
	JString		organization;
	int			uid;
	int			gid;
	int			privilege;
	int			operation;
	bool		securityAttach;
	bool		authenticator;
	bool		authenticating;
	
public:
	virtual const char* getUserName(void);
	virtual const char* getFirstName(void);
	virtual const char* getMiddleName(void);
	virtual const char* getLastName(void);
	virtual const char* getFullName(void);
	virtual const char* getSystemName(void);
	virtual const char* getGroupName(void);
	virtual void setUserName(const char* name);
	virtual void setFirstName(const char* name);
	virtual void setMiddleName(const char* name);
	virtual void setLastName(const char* name);
	virtual void setFullName(const char* name);
	virtual void setSystemName(const char* name);
	virtual void setGroupName(const char* name);
	virtual int getUID(void);
	virtual int getGID(void);
	virtual void setUID(int value);
	virtual void setGID(int value);
	UserData(const char* user);
	const char* getPassword(void);
	UserData(int apbLength, const UCHAR* apb);
	
protected:
	void init(void);
	
public:
	void processApbItem(const UCHAR* item);
	virtual void parseDpb(int dpbLength, const UCHAR* dpb);
	virtual int parseApb(int apbLength, const UCHAR* apb);
	virtual void processDpbItem(int type, int length, const UCHAR* data);
	int getShort(const UCHAR **ptr);
	int getInt(const UCHAR** ptr);
	int getNumber(int length, const UCHAR* ptr);
	JString getString(int length, const UCHAR* data);
	void processUserInfo(const UCHAR* userInfo);
	virtual void processUserInfoItem(int type, int length, const UCHAR* data);
	JString getOldPasswordHash(void);
};

#endif
