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
#include "UserData.h"
#include "ibase.h"
#include "enc_proto.h"

#define PASSWORD_SALT  "9z"

UserData::UserData(void)
{
	init();
}

UserData::UserData(const char* user)
{
	init();
	userName = user;
}

void UserData::init(void)
{
	gid = 0;
	uid = 0;
	securityAttach = false;
}

UserData::~UserData(void)
{
}

const char* UserData::getUserName(void)
{
	return userName;
}

const char* UserData::getFirstName(void)
{
	return firstName;
}

const char* UserData::getMiddleName(void)
{
	return middleName;
}

const char* UserData::getLastName(void)
{
	return lastName;
}

const char* UserData::getFullName(void)
{
	return fullName;
}

const char* UserData::getSystemName(void)
{
	return systemName;
}

const char* UserData::getGroupName(void)
{
	return groupName;
}

void UserData::setUserName(const char* name)
{
	userName = name;
}

void UserData::setFirstName(const char* name)
{
	firstName = name;
}

void UserData::setMiddleName(const char* name)
{
	middleName = name;
}

void UserData::setLastName(const char* name)
{
	lastName = name;
}

void UserData::setFullName(const char* name)
{
	fullName = name;
}

void UserData::setSystemName(const char* name)
{
	systemName = name;
}

void UserData::setGroupName(const char* name)
{
	groupName = name;
}

int UserData::getUID(void)
{
	return uid;
}

int UserData::getGID(void)
{
	return gid;
}

void UserData::setUID(int value)
{
	uid = value;
}

void UserData::setGID(int value)
{
	gid = value;
}

const char* UserData::getPassword(void)
{
	return NULL;
}

int UserData::parseApb(int apbLength, const UCHAR* apb)
{
	const UCHAR *p = apb;
	int version = *p++;
	operation = 0;
	
	for (const UCHAR *end = apb + apbLength; p < end;)
		{
		processApbItem(p);
		p += 2 + p[1];
		}
	
	return operation;
}

void UserData::processApbItem(const UCHAR* item)
{
	int code = *item++;
	int length = *item++;
	
	switch (code)
		{
		case fb_apb_uid:
			uid = getNumber(length, item);
			break;
			
		case fb_apb_gid:
			gid = getNumber(length, item);
			break;
			
		case fb_apb_operation:
			operation = getNumber(length, item);
			break;
			
		case fb_apb_account:
			userName = getString(length, item);
			break;
			
		case fb_apb_password:
			password = getString(length, item);
			break;
			
		case fb_apb_group:
			groupName = getString(length, item);
			break;
			
		case fb_apb_first_name:
			firstName = getString(length, item);
			break;
			
		case fb_apb_middle_name:
			middleName = getString(length, item);
			break;
			
		case fb_apb_last_name:
			lastName = getString(length, item);
			break;
		}
}

void UserData::parseDpb(int dpbLength, const UCHAR* dpb)
{
	const UCHAR *p = dpb;
	int version = *p++;
	
	for (const UCHAR *end = dpb + dpbLength; p < end;)
		{
		int type = *p++;
		int length = *p++;
		processDpbItem(type, length, p);
		p += length;
		}

}

void UserData::processDpbItem(int type, int length, const UCHAR* data)
{
	switch (type)
		{
		case isc_dpb_sys_user_name:
			systemName = getString(length, data);
			break;
			
		case isc_dpb_password:
			password = getString(length, data);
			break;
			
		case isc_dpb_user_name:
			userName = getString(length, data);
			break;
			
		case isc_dpb_password_enc:
			encryptedPassword = getString(length, data);
			break;
		
		case isc_dpb_sec_attach:
			securityAttach = getNumber(length, data) != 0;
			break;				
		}	
}

int UserData::getShort(const UCHAR **ptr)
{
	int value = (short)((*ptr)[0] | (*ptr)[1] << 8);
	*ptr += 2;
	
	return value;
}

int UserData::getInt(const UCHAR** ptr)
{
	int value = getShort(ptr);
	value |= getShort(ptr) << 16;
	
	return value;
}

int UserData::getNumber(int length, const UCHAR* ptr)
{
	switch (length)
		{
		case 1:
			return ptr[0];
		
		case 2:
			return getShort(&ptr);
		
		case 4:
			return getInt(&ptr);
		}
	
	return -1;	
	
}

JString UserData::getString(int length, const UCHAR* data)
{
	return JString((const char*) data, length);
}

void UserData::processUserInfo(const UCHAR* userInfo)
{
	for (const UCHAR *p = userInfo; *p != isc_info_end;)
		{
		int type = *p++;
		int length = getShort(&p);
		processUserInfoItem(type, length, p);
		p += length;
		}
}

void UserData::processUserInfoItem(int type, int length, const UCHAR* data)
{
	switch (type)
		{
		case fb_info_user_account:
			userName = getString(length, data);
			break;
		
		case fb_info_user_password:
			password = getString(length, data);
			break;
		
		case fb_info_user_group:
			groupName = getString(length, data);
			break;
		
		case fb_info_user_first_name:
			firstName = getString(length, data);
			break;
		
		case fb_info_user_middle_name:
			middleName = getString(length, data);
			break;
		
		case fb_info_user_last_name:
			lastName = getString(length, data);
			break;
		
		case fb_info_user_uid:
			uid = getNumber(length, data);
			break;
		
		case fb_info_user_gid:
			gid = getNumber(length, data);
			break;
		
		case isc_infunk:
			type = *data++;
			break;
		}
}

JString UserData::getOldPasswordHash(void)
{
	TEXT pw1 [ENCRYPT_SIZE];
	TEXT pw2 [ENCRYPT_SIZE];
	
	if (encryptedPassword.IsEmpty())
		{
		ENC_crypt(pw1, sizeof(pw2), password, PASSWORD_SALT);
		ENC_crypt(pw2, sizeof(pw2), pw1 + 2, PASSWORD_SALT);
		}
	else
		ENC_crypt(pw2, sizeof(pw2), encryptedPassword, PASSWORD_SALT);
		
	
	return pw2 + 2;
}
