/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pwd.e
 *	DESCRIPTION:	User information database access
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2003.02.02 Dmitry Yemanov: Implemented cached security database connection
 */

#include "fbdev.h"
#include <string.h>
#include <stdlib.h>
//#include "../jrd/y_ref.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/enc_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/sch_proto.h"
#include "../jrd/thd_proto.h"
#include "Sync.h"

#ifdef SUPERSERVER
const bool SecurityDatabase::is_cached = true;
#else
const bool SecurityDatabase::is_cached = false;
#endif

// BLR to search database for user name record

const UCHAR SecurityDatabase::PWD_REQUEST[256] = {
	blr_version5,
	blr_begin,
	blr_message, 1, 4, 0,
	blr_long, 0,
	blr_long, 0,
	blr_short, 0,
	blr_text, 34, 0,
	blr_message, 0, 1, 0,
	blr_cstring, 129, 0,
	blr_receive, 0,
	blr_begin,
	blr_for,
	blr_rse, 1,
	blr_relation, 5, 'U', 'S', 'E', 'R', 'S', 0,
	blr_first,
	blr_literal, blr_short, 0, 1, 0,
	blr_boolean,
	blr_eql,
	blr_field, 0, 9, 'U', 'S', 'E', 'R', '_', 'N', 'A', 'M', 'E',
	blr_parameter, 0, 0, 0,
	blr_end,
	blr_send, 1,
	blr_begin,
	blr_assignment,
	blr_field, 0, 3, 'G', 'I', 'D',
	blr_parameter, 1, 0, 0,
	blr_assignment,
	blr_field, 0, 3, 'U', 'I', 'D',
	blr_parameter, 1, 1, 0,
	blr_assignment,
	blr_literal, blr_short, 0, 1, 0,
	blr_parameter, 1, 2, 0,
	blr_assignment,
	blr_field, 0, 6, 'P', 'A', 'S', 'S', 'W', 'D',
	blr_parameter, 1, 3, 0,
	blr_end,
	blr_send, 1,
	blr_assignment,
	blr_literal, blr_short, 0, 0, 0,
	blr_parameter, 1, 2, 0,
	blr_end,
	blr_end,
	blr_eoc
};

// Transaction parameter buffer

const UCHAR SecurityDatabase::TPB[4] = {
	isc_tpb_version1,
	isc_tpb_read,
	isc_tpb_concurrency,
	isc_tpb_wait
};

// Static instance of the database

SecurityDatabase SecurityDatabase::instance;


/******************************************************************************
 *
 *	Private interface
 */

void SecurityDatabase::fini()
{
	counter -= (is_cached) ? 1 : 0;
#ifndef EMBEDDED

	if (counter == 1 && lookup_db)
		{
		THREAD_EXIT;
		isc_detach_database(status, &lookup_db);
		THREAD_ENTER;
		}
		
#endif
}

void SecurityDatabase::init()
{
	counter += (is_cached) ? 1 : 0;
}

bool SecurityDatabase::lookup_user(const TEXT* securityDatabase, TEXT * user_name, int *uid, int *gid, TEXT * pwd)
{
	Sync sync(&syncObject, "SecurityDatabase::lookup_user");
	sync.lock(Exclusive);
	bool found = false;		// user found flag
	TEXT uname[129];		// user name buffer
	user_record user;		// user record

	// Start by clearing the output data

	if (uid)
		*uid = 0;
	if (gid)
		*gid = 0;
	if (pwd)
		*pwd = '\0';

	strncpy(uname, user_name, 129);

	// Attach database and compile request

	if (!prepare(securityDatabase))
		{
		if (lookup_db)
			isc_detach_database(status, &lookup_db);
		THREAD_ENTER;
		ERR_post(isc_psw_attach, 0);
		}

	// Lookup

	isc_tr_handle lookup_trans = 0;

	if (isc_start_transaction(status, &lookup_trans, 1, &lookup_db, sizeof(TPB), TPB))
		{
		THREAD_ENTER;
		ERR_post(isc_psw_start_trans, 0);
		}

	if (!isc_start_and_send(status, &lookup_req, &lookup_trans, 0, sizeof(uname), uname, 0))
		{
		while (1)
			{
			isc_receive(status, &lookup_req, 1, sizeof(user), &user, 0);
			if (!user.flag || status[1])
				break;
			found = true;
			if (uid)
				*uid = user.uid;
			if (gid)
				*gid = user.gid;
			if (pwd)
				strncpy(pwd, user.password, 32);
			}
		}

	isc_rollback_transaction(status, &lookup_trans);

	if (!is_cached)
		isc_detach_database(status, &lookup_db);

	THREAD_ENTER;

	return found;
}

bool SecurityDatabase::prepare(const TEXT* securityDatabase)
{
	TEXT user_info_name[MAXPATHLEN];

	if (lookup_db)
		{
		THREAD_EXIT;
		return true;
		}

	// Register as internal database handle

	/***
	IHNDL ihandle;

	for (ihandle = internal_db_handles; ihandle; ihandle = ihandle->ihndl_next)
		{
		if (ihandle->ihndl_object == NULL)
			{
			ihandle->ihndl_object = &lookup_db;
			break;
			}
		}

	if (!ihandle)
		{
		ihandle = (IHNDL) gds__alloc ((SLONG) sizeof(struct ihndl));
		ihandle->ihndl_object = &lookup_db;
		ihandle->ihndl_next = internal_db_handles;
		internal_db_handles = ihandle;
		}
	***/
	
	THREAD_EXIT;

	lookup_db = lookup_req = 0;

	// Initialize the database name

	//getPath(user_info_name);
	//strcpy (user_info_name, USER_INFO_NAME);
	strcpy (user_info_name, securityDatabase);
	
	// Perhaps build up a dpb

	SCHAR dpb_buffer[256];
	SCHAR *dpb = dpb_buffer;
	*dpb++ = isc_dpb_version1;

	// Insert username

	static const char szAuthenticator[] = "AUTHENTICATOR";
	const size_t nAuthNameLen = strlen(szAuthenticator);
	*dpb++ = isc_dpb_user_name;
	*dpb++ = (UCHAR) nAuthNameLen;
	memcpy(dpb, szAuthenticator, nAuthNameLen);
	dpb += nAuthNameLen;

	// Insert password

	static const char szPassword[] = "none";
	const size_t nPswdLen = strlen(szPassword);
	*dpb++ = isc_dpb_password;
	*dpb++ = (UCHAR) nPswdLen;
	memcpy(dpb, szPassword, nPswdLen);
	dpb += nPswdLen;

	*dpb++ = isc_dpb_sec_attach;	// Attachment is for the security database
	*dpb++ = 1;						// Parameter value length
	*dpb++ = TRUE;					// Parameter value

	int dpb_len = dpb - dpb_buffer;

	isc_attach_database(status, 0, user_info_name, &lookup_db, dpb_len, dpb_buffer);

	if (status[1] == isc_login)
		{
		// We may be going against a V3 database which does not
		// understand this combination
		isc_attach_database(status, 0, user_info_name, &lookup_db, 0, 0);
		}

	//fb_assert(ihandle->ihndl_object == &lookup_db);
	//ihandle->ihndl_object = NULL;

	isc_compile_request(status, &lookup_db, &lookup_req, sizeof(PWD_REQUEST), (SCHAR*) PWD_REQUEST);

	if (status[1])
		return false;

	return true;
}

/******************************************************************************
 *
 *	Public interface
 */

void SecurityDatabase::getPath(TEXT* path_buffer)
{
	gds__prefix(path_buffer, USER_INFO_NAME);
}

void SecurityDatabase::initialize()
{
	instance.init();
}

void SecurityDatabase::shutdown()
{
	instance.fini();
}

void SecurityDatabase::verifyUser(const TEXT* securityDatabase,
								  TEXT* name,
								  TEXT* user_name,
								  TEXT* password,
								  TEXT* password_enc,
								  int* uid,
								  int* gid,
								  int* node_id)
{
	if (user_name)
		{
		TEXT* p = name;
		for (const TEXT* q = user_name; *q; ++q, ++p)
			*p = UPPER7(*q);
		*p = 0;
		}

#ifndef EMBEDDED

	// Look up the user name in the userinfo database and use the parameters
	// found there. This means that another database must be accessed, and
	// that means the current context .must be saved and restored.

	TEXT pw1[33];
	bool found = instance.lookup_user(securityDatabase, name, uid, gid, pw1);

	// Punt if the user has specified neither a raw nor an encrypted password,
	// or if the user has specified both a raw and an encrypted password, 
	// or if the user name specified was not in the password database
	// (or if there was no password database - it's still not found)

	if ((!password && !password_enc) || (password && password_enc) || !found)
		ERR_post(isc_login, 0);

	TEXT pwt[ENCRYPT_SIZE];
	
	if (password) 
		{
		//strcpy(pwt, ENC_crypt(password, PASSWORD_SALT));
		ENC_crypt(pwt, sizeof(pwt), password, PASSWORD_SALT);
		password_enc = pwt;
		}
		
	TEXT pw2[ENCRYPT_SIZE];
	//strcpy(pw2, ENC_crypt(password_enc, PASSWORD_SALT));
	ENC_crypt(pw2, sizeof(pw2), password_enc + 2, PASSWORD_SALT);
	
	if (strncmp(pw1, pw2 + 2, 11))
		ERR_post(isc_login, 0);

#endif

	*node_id = 0;
}
