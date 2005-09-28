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
 
#include <string.h>

#ifdef MVS
#include <strings.h> // for strcasecmp
#endif

#include "fbdev.h"
#include "common.h"
#include "SecurityDb.h"
#include "ConfObject.h"
#include "Parameters.h"
#include "UserData.h"
#include "Connection.h"
#include "PStatement.h"
#include "RSet.h"
#include "OSRIException.h"
#include "InfoGen.h"
#include "PBGen.h"
#include "enc_proto.h"

#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif
#endif

SecurityDb::SecurityDb(SecurityPlugin *securityChain) : SecurityPlugin(securityChain)
{
	databaseName = configuration->getValue(SecurityDatabase, SecurityDatabaseValue);
	self = databaseName.equalsNoCase("SELF");
	none = databaseName.equalsNoCase("NONE") || databaseName.IsEmpty();
	dbHandle = 0;
}

SecurityDb::~SecurityDb(void)
{
	if (dbHandle)
		{
		ISC_STATUS statusVector [20];
		isc_detach_database(statusVector, &dbHandle);
		dbHandle = 0;
		}
}

void SecurityDb::updateAccountInfo(SecurityContext *context, int apbLength, const UCHAR* apb)
{
	if (none)
		return;
	
	if (!self)
		{
		if (!dbHandle)
			attachDatabase();
		
		ISC_STATUS statusVector [20];
		
		if (fb_update_account_info(statusVector, &dbHandle, apbLength, apb))
			throw OSRIException(statusVector);
		
		return;
		}
		
	UserData userData;
	userData.parseApb(apbLength, apb);
	Connection *connection = context->getConnection();
	PStatement statement;
	JString encryptedPassword = userData.getOldPasswordHash();
	int n = 1;
	
	switch (userData.operation)
		{
		case fb_apb_update_account:
		case fb_apb_upgrade_account:
		case fb_apb_create_account:
			statement = connection->prepareStatement(
				"insert into users (user_name, passwd, uid, gid)values(?,?,?,?)");
			statement->setString(n++, JString::upcase(userData.userName));
			statement->setString(n++, encryptedPassword);
			statement->setInt(n++, userData.uid);
			statement->setInt(n++, userData.gid);
			break;
			
		case fb_apb_delete_account:
			statement = connection->prepareStatement(
				"delete from users where user_name=?");
			statement->setString(n++, userData.userName);
			break;
			
		}
	
	statement->executeUpdate();	
}

void SecurityDb::authenticateUser(SecurityContext *context, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	UserData userData;
	userData.parseDpb(dpbLength, dpb);

	// Handle connection to security database
	
	if (!self && !none)
		{
		if (userData.securityAttach)
			throw OSRIException(isc_login, 0);
			
		if (!dbHandle)
			{
			if (databaseName == context->getDatabaseFilename())
				throw OSRIException(isc_login, 0);
			attachDatabase();
			}
		
		ISC_STATUS statusVector [20];
		
		if (fb_authenticate_user(statusVector, &dbHandle, dpbLength, dpb, itemsLength, items, bufferLength, buffer))
			throw OSRIException(statusVector);
		
		return;
		}
	
	// Either self authenticating or no authentication at all.  In either case, set up 
	// to generate user information
		
	InfoGen info(buffer, bufferLength);
	JString accountName = JString::upcase(userData.userName);
	
	// If none, just give back the user name and forget about it
	
	if (none)
		{
		for (int n = 0; n < itemsLength; ++n)
			{
			int item = items[n];
			switch (item)
				{
				case fb_info_user_account:
					info.putString(item, accountName);
					break;

				default:
					info.putUnknown(item);
				}
			}
		
		info.fini();
		return;
		}
	
	// We've got some work to do.
	
	Connection *connection = context->getConnection();
	PStatement statement = connection->prepareStatement(
		"select * from users where user_name=?");
	statement->setString(1, accountName);
	RSet resultSet = statement->executeQuery();
	JString oldHash = userData.getOldPasswordHash();
	int hit = false;
	
	if (resultSet->next())
		{
		hit = true;
		const char *password = resultSet->getString("PASSWD");

		if (oldHash != password)		
			throw OSRIException(isc_login, 0);

		for (int n = 0; n < itemsLength; ++n)
			{
			int item = items[n];
			switch (item)
				{
				case fb_info_user_account:
					info.putString(item, accountName);
					break;

				case fb_info_user_password:
					info.putString(item, userData.password);
					break;

				case fb_info_user_group:
					info.putString(item, resultSet->getString("GROUP_NAME"));
					break;

				case fb_info_user_first_name:
					info.putString(item, resultSet->getString("FIRST_NAME"));
					break;

				case fb_info_user_middle_name:
					info.putString(item, resultSet->getString("MIDDLE_NAME"));
					break;

				case fb_info_user_last_name:
					info.putString(item, resultSet->getString("LAST_NAME"));
					break;

				case fb_info_user_uid:
					info.putInt(item, resultSet->getInt("UID"));
					break;
				
				case fb_info_user_gid:
					info.putInt(item, resultSet->getInt("GID"));
					break;
				
				default:
					info.putUnknown(item);
				}
			}
		}
	
	// If we didn't find anything, check for short term hack for authenticator
	
	if (!hit)
		{
		if (strcasecmp(accountName, "AUTHENTICATOR") != 0)
			throw OSRIException(isc_login, 0);
		
		userData.authenticator = true;
		
		if (strcmp(userData.password, "none") != 0)
			{
			TEXT pw1 [ENCRYPT_SIZE];
			ENC_crypt(pw1, sizeof(pw1), "none", PASSWORD_SALT);
			
			if (strcmp(pw1, userData.encryptedPassword) != 0)
				throw OSRIException(isc_login, 0);
			}
			
		for (int n = 0; n < itemsLength; ++n)
			{
			int item = items[n];
			switch (item)
				{
				case fb_info_user_account:
					info.putString(item, accountName);
					break;

				case fb_info_user_password:
					info.putString(item, userData.password);
					break;

				default:
					info.putUnknown(item);
				}
			}
		}
		
	info.fini();
}

void SecurityDb::attachDatabase(void)
{
	PBGen gen(isc_dpb_version1);
	gen.putParameter(isc_dpb_user_name, "AUTHENTICATOR");
	gen.putParameter(isc_dpb_password, "none");
	gen.putParameter(isc_dpb_sec_attach, 1);
	ISC_STATUS statusVector [20];
	
	if (isc_attach_database(statusVector, 0, databaseName, &dbHandle, gen.getLength(), (char*) gen.buffer))
		throw OSRIException(statusVector);
}
