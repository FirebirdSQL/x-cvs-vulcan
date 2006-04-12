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
#include "Connect.h"

#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif
#endif

static const char *creationDDL [] = 
	{
	"create domain rdb$user_name varchar(128) CHARACTER SET UNICODE_FSS;",
	"create domain rdb$uid integer;",
	"create domain rdb$gid integer;",
	"create domain rdb$password varchar(64) CHARACTER SET ASCII;",
	"create domain rdb$user_privilege integer;",
	"create domain rdb$comment BLOB sub_type TEXT segment size 80 CHARACTER SET UNICODE_FSS;",
	"create domain rdb$name_part varchar(128) CHARACTER SET UNICODE_FSS;",

	"create table rdb$users(\n"
		"rdb$user_name		rdb$user_name not null primary key,\n"
		"rdb$sys_user_name	rdb$user_name,\n"
		"rdb$group_name		rdb$user_name,\n"
		"rdb$uid			rdb$uid,\n"
		"rdb$gid			rdb$gid,\n"
		"rdb$password		rdb$password,\n"
		"rdb$privilege		rdb$user_privilege,\n"
		"rdb$comment		rdb$comment,\n"
		"rdb$first_name		rdb$name_part,\n"
		"rdb$middle_name	rdb$name_part,\n"
		"rdb$last_name		rdb$name_part);",
	
	"grant all on rdb$users to sysdba with grant option",	
	NULL
	};

SecurityDb::SecurityDb(SecurityPlugin *securityChain) : SecurityPlugin(securityChain)
{
	databaseName = configuration->getValue(SecurityDatabase, SecurityDatabaseValue);
	self = databaseName.equalsNoCase("SELF");
	none = databaseName.equalsNoCase("NONE") || databaseName.IsEmpty();
	dbHandle = 0;
	authenticate = NULL;
	connection = NULL;
	haveTable = false;
}

SecurityDb::~SecurityDb(void)
{
	if (authenticate)
		authenticate->close();
		
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
		isc_db_handle handle = 0;
		PBGen gen(isc_dpb_version1);
		gen.putParameter(isc_dpb_user_name, context->getAccount());
		const char *password = context->getEncryptedPassword();
		
		if (password && password[0])
			gen.putParameter(isc_dpb_password_enc, password);
		else if ((password = context->getPassword()) && password[0])
			gen.putParameter(isc_dpb_password, password);
			
		ISC_STATUS statusVector [20];
		ISC_STATUS statusVector2 [20];

		if (isc_attach_database(statusVector, 0, databaseName, &handle, gen.getLength(), (char*) gen.buffer))
			throw OSRIException(statusVector);

		fb_update_account_info(statusVector, &handle, apbLength, apb);
		isc_detach_database(statusVector2, &handle);
		
		if (statusVector[1])
			throw OSRIException(statusVector);
		
		return;
		}
		
	UserData userData;
	userData.parseApb(apbLength, apb);
	Connect userConnection = (InternalConnection*) context->getUserConnection();
	Connect connection = (InternalConnection*) userConnection->clone();
	
	try
		{	
		PStatement statement;
		
		if (!haveTable && !(haveTable = checkUsersTable(connection)))
			{
			for (const char **ddl = creationDDL; *ddl; ++ddl)
				{
				statement = connection->prepareStatement(*ddl);
				statement->execute();
				}
			
			connection->commit();
			}
			
		JString encryptedPassword = userData.getOldPasswordHash();
		int n = 1;
		int count = 0;
		
		switch (userData.operation)
			{
			case fb_apb_update_account:
			case fb_apb_upgrade_account:
				statement = connection->prepareStatement(
					"update rdb$users set rdb$password=?, rdb$uid=?, rdb$gid=?"
					"  where rdb$user_name=?");
				statement->setString(n++, encryptedPassword);
				statement->setInt(n++, userData.uid);
				statement->setInt(n++, userData.gid);
				statement->setString(n++, JString::upcase(userData.userName));
				count = statement->executeUpdate();	
				
				if (count || userData.operation == fb_apb_update_account)
					break;
				
			case fb_apb_create_account:
				statement = connection->prepareStatement(
					"insert into rdb$users (rdb$user_name, rdb$password, rdb$uid, rdb$gid) values (?,?,?,?)");
				statement->setString(n++, JString::upcase(userData.userName));
				statement->setString(n++, encryptedPassword);
				statement->setInt(n++, userData.uid);
				statement->setInt(n++, userData.gid);
				count = statement->executeUpdate();	
			break;
				
			case fb_apb_delete_account:
				statement = connection->prepareStatement(
					"delete from rdb$users where rdb$user_name=?");
				statement->setString(n++, userData.userName);
				count = statement->executeUpdate();	
				break;
				
			}
		
		connection->commit();
		}
	
	catch (...)
		{
		connection->rollback();
		throw;
		}
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

	if (!none && !connection)
		//connection = context->getUserConnection();
		connection = context->getNewConnection();
		
	// If none, just give back the user name and forget about it
	
	if (none || (!haveTable && !checkUsersTable(connection)))
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
	
	PStatement statement = connection->prepareStatement( "select * from rdb$users where rdb$user_name=?");
	
	/***
	if (!authenticate)
		authenticate = connection->prepareStatement( "select * from users where user_name=?");
	***/
	
	statement->setString(1, accountName);
	RSet resultSet = statement->executeQuery();
	JString oldHash = userData.getOldPasswordHash();
	int hit = false;
	
	if (resultSet->next())
		{
		hit = true;
		const char *password = resultSet->getString("RDB$PASSWORD");

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
					info.putString(item, resultSet->getString("RDB$GROUP_NAME"));
					break;

				case fb_info_user_first_name:
					info.putString(item, resultSet->getString("RDB$FIRST_NAME"));
					break;

				case fb_info_user_middle_name:
					info.putString(item, resultSet->getString("RDB$MIDDLE_NAME"));
					break;

				case fb_info_user_last_name:
					info.putString(item, resultSet->getString("RDB$LAST_NAME"));
					break;

				case fb_info_user_uid:
					info.putInt(item, resultSet->getInt("RDB$UID"));
					break;
				
				case fb_info_user_gid:
					info.putInt(item, resultSet->getInt("RDB$GID"));
					break;
				
				case fb_info_user_authenticator:
					info.putShort(item, false);
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

				case fb_info_user_authenticator:
					info.putShort(item, true);
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

bool SecurityDb::checkUsersTable(Connection* connection)
{
	PStatement statement = connection->prepareStatement(
		"select rdb$relation_id from rdb$relations where rdb$relation_name='RDB$USERS'");
	RSet resultSet = statement->executeQuery();
	
	return resultSet->next();
}

void SecurityDb::close(void)
{
	if (authenticate)
		{
		authenticate->close();
		authenticate = NULL;
		}
	
	if (connection)
		{
		connection->close();
		connection = NULL;
		}
}
