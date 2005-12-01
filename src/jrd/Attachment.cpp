/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Attachment.cpp
 *	DESCRIPTION:	Database Attachment Class
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
 * December 28, 2003	Created by James A. Starkey
 *
 */

// Attachment.cpp: implementation of the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "Attachment.h"
#include "../dsql/Cursor.h"
#include "../dsql/DStatement.h"
#include "InternalConnection.h"
#include "Sync.h"
#include "InternalConnection.h"
#include "all.h"
#include "tra.h"
#include "UserData.h"
#include "lck_proto.h"
#include "event_proto.h"
#include "../jrd/met_proto.h"
#include "CompilerScratch.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/scl_proto.h"
#include "Relation.h"
#include "scl.h"
#include "jrd.h"

// User info items

static const UCHAR userInfoItems [] = {
	fb_info_user_account,
	fb_info_user_uid,
	fb_info_user_gid,
	fb_info_user_password,
	fb_info_user_group,
	fb_info_user_first_name,
	fb_info_user_middle_name,
	fb_info_user_last_name,
	fb_info_user_authenticator
	};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Attachment::Attachment(Database *database)
{
	att_database = database;
	att_filename = att_database->dbb_filename;
	cursors = NULL;
	firstConnection = lastConnection = NULL;
	att_next = NULL;
	att_blocking = NULL;
	//att_user = NULL;
	att_transactions = NULL;
	att_dbkey_trans = NULL;
	att_requests = NULL;
	att_id_lock = NULL;
	att_security_class = NULL;
	att_security_classes = NULL;
	att_relation_locks = NULL;
	att_record_locks = NULL;
	att_event_session = 0;
	//att_bookmarks = NULL;
	//att_bkm_quick_ref = NULL;
	//att_lck_quick_ref = NULL;
	att_lc_messages = NULL;
	att_long_locks = NULL;
	att_compatibility_table = NULL;
	att_val_errors = NULL;
	att_active_sorts = NULL;
	memset (att_counts, 0, sizeof (att_counts));
	att_flags = 0;
	att_charset = 0;
	att_lock_owner_handle = 0;
	userFlags = 0;
}

Attachment::~Attachment()
{
	/***
	if (att_user) 
		delete att_user;
	
	for (Bookmark* bookmark; bookmark = att_bookmarks;) 
		{
		att_bookmarks = bookmark->bkm_next;
		delete bookmark;
		}
		
	if (att_bkm_quick_ref)
		delete att_bkm_quick_ref;
			
	if (att_lck_quick_ref)
		delete att_lck_quick_ref;
	***/

	while (firstConnection)
		firstConnection->close();
}

Cursor* Attachment::allocateCursor(DStatement* statement, Transaction* transaction)
{
	Cursor *cursor = new Cursor (statement, transaction);
	cursor->next = cursors;
	cursors = cursor;
	
	return cursor;
}

DStatement* Attachment::allocateStatement(void)
{
	return new DStatement (this);
}

InternalConnection* Attachment::getUserConnection(Transaction* transaction)
{
	InternalConnection *connection = new InternalConnection (this, transaction);
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "Attachment::getUserConnection");
	sync.lock (Exclusive);
#endif
	
	return connection;
}

void Attachment::closeConnection(InternalConnection* connection)
{
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "Attachment::closeConnection");
	sync.lock (Exclusive);
#endif

	if (connection->prior)
		connection->prior->next = connection->next;
	else
		firstConnection = connection->next;
	
	if (connection->next)
		connection->next->prior = connection->prior;
	else
		lastConnection = connection->prior;
}

bool Attachment::isSoleAttachment(void)
{
	return att_database->dbb_attachments == this && !att_next;
}

Cursor* Attachment::findCursor(const char* name)
{
	for (Cursor *cursor = cursors; cursor; cursor = cursor->next)
		if (cursor->name == name)
			return cursor;
	
	return NULL;
}

void Attachment::deleteCursor(Cursor* cursor)
{
	for (Cursor **ptr = &cursors; *ptr; ptr = &(*ptr)->next)
		if (*ptr == cursor)
			{
			*ptr = cursor->next;
			break;
			}
	
	delete cursor;
}

void Attachment::endTransaction(Transaction* transaction)
{
	for (Cursor *cursor; cursor = cursors;)
		cursor->statement->clearCursor();

	for (Transaction **ptr = &att_transactions; *ptr; ptr = &(*ptr)->tra_next) 
		if (*ptr == transaction) 
			{
			*ptr = transaction->tra_next;
			break;
			}
}

void Attachment::updateAccountInfo(thread_db* tdbb, int apbLength, const UCHAR* apb)
{
	att_database->updateAccountInfo(tdbb, this, apbLength, apb);
}

void Attachment::authenticateUser(thread_db* tdbb, int dpbLength, const UCHAR* dpb)
{
	UCHAR buffer[256];
	att_database->authenticateUser(tdbb, dpbLength, dpb, sizeof(userInfoItems), userInfoItems, sizeof (buffer), buffer);
	userData.processUserInfo(buffer);
	const UCHAR *p = dpb;
	int version = *p++;
	
	for (const UCHAR *end = dpb + dpbLength; p < end;)
		{
		int type = *p++;
		int length = *p++;
		
		if (type == isc_dpb_sql_role_name)
			userData.roleName = JString((const char*) p, length);

		p += length;
		}
}

void Attachment::shutdown(thread_db* tdbb)
{
	if (att_event_session)
		EVENT_delete_session(att_event_session);

	if (att_id_lock)
		LCK_release(att_id_lock);

	for (vcl** vector = att_counts; vector < att_counts + DBB_max_count; ++vector)
		if (*vector)
			delete *vector;

	if (att_lc_messages)
		delete att_lc_messages;

	/* Release any validation error vector allocated */

	if (att_val_errors) 
		{
		delete att_val_errors;
		att_val_errors = NULL;
		}

	/* Release the persistent locks taken out during the attachment */
	
	vec* lock_vector = att_relation_locks;
	
	if (lock_vector)
		{
		size_t i = 0;
		for (vec::iterator lock = lock_vector->begin(); i < lock_vector->count(); i++, lock++)
			{
			if (*lock)
				{
				LCK_release((Lock*)(*lock));
				delete *lock;
				}
			}
		delete lock_vector;
		}

    Lock* record_lock;
    
	for (record_lock = att_record_locks; record_lock; record_lock = record_lock->lck_att_next)
		LCK_release(record_lock);

	/* bug #7781, need to null out the attachment pointer of all locks which
	   were hung off this attachment block, to ensure that the attachment
	   block doesn't get dereferenced after it is released */

#ifdef SHARED_CACHE
	Sync sync(&syncLongLocks, "Attachment::shutdown");
	sync.lock(Exclusive);
#endif

	for (record_lock = att_long_locks; record_lock; record_lock = record_lock->lck_next)
		record_lock->lck_attachment = NULL;

	if (att_flags & ATT_lck_init_done)
		LCK_fini(tdbb, LCK_OWNER_attachment);	/* For the attachment */

	if (att_compatibility_table)
		delete att_compatibility_table;

	for (Request* request; request = att_requests;) 
		CMP_release(tdbb, request);
	
	for (SecurityClass* sec_class; sec_class = att_security_classes;)
		SCL_release(tdbb, sec_class);
}

void Attachment::addLongLock(Lock* lock)
{
#ifdef SHARED_CACHE
	Sync sync(&syncLongLocks, "Attachment::addLongLock");
	sync.lock(Exclusive);
#endif
	
/* check to see if lock is already here ???? */

#ifdef DEV_BUILD
    for (Lock *t = att_long_locks; t; t = t->lck_next)
	    if (t == lock) 
			fb_assert(false);
#endif
	
	if (lock->lck_next = att_long_locks)
		lock->lck_next->lck_prior = lock;
		
	lock->lck_prior = NULL;
	att_long_locks = lock;
	lock->lck_long_lock = true;
}

void Attachment::removeLongLock(Lock* lock)
{
#ifdef SHARED_CACHE
	Sync sync(&syncLongLocks, "Attachment::removeLockLock");
	sync.lock(Exclusive);
#endif

	if (lock->lck_prior)
		{
		if (lock->lck_prior->lck_next = lock->lck_next)
			lock->lck_next->lck_prior = lock->lck_prior;
		}
	else if (lock->lck_next)
		{
		lock->lck_next->lck_prior = NULL;
		att_long_locks = lock->lck_next;
		}
	else if (att_long_locks == lock)
		att_long_locks = NULL;
	else
		fb_assert(false);
	
	lock->lck_next = NULL;
	lock->lck_prior = NULL;
	lock->lck_attachment = NULL;
	lock->lck_long_lock = false;
}

Lock* Attachment::findBlock(Lock* lock, int level)
{
#ifdef SHARED_CACHE
	Sync sync(&syncLongLocks, "Attachment::findBlock");
	sync.lock(Shared);
#endif

	for (Lock* next = att_long_locks; next; next = next->lck_next)
		if (lock->lck_attachment != next->lck_attachment &&
			 next->equiv(lock) && !next->compatible(lock, level))
			return next;

	return NULL;
}

Relation* Attachment::findRelation(thread_db* tdbb, int relationId, int csbFlags)
{
	Relation *relation = MET_lookup_relation_id(tdbb, relationId, FALSE);

	if (relation)
		relation->scanRelation(tdbb, csbFlags);

	return relation;
}

Relation* Attachment::getRelation(thread_db* tdbb, int relationId)
{
	Relation *relation = findRelation(tdbb, relationId);
	
	if (!relation)
		{
		char name[32];
		sprintf(name, "id %d", relationId);
		throw OSRIException(isc_relnotdef, isc_arg_string, name, 0);
		}
	
	return relation;
}

Relation* Attachment::findRelation(thread_db* tdbb, const char* relationName, int csbFlags)
{
	Relation *relation = MET_lookup_relation(tdbb, relationName);

	if (relation)
		relation->scanRelation(tdbb, csbFlags);

	return relation;
}

Relation* Attachment::getRelation(thread_db* tdbb, const char* relationName)
{
	Relation *relation = findRelation(tdbb, relationName);
	
	if (!relation)
		throw OSRIException(isc_relnotdef, isc_arg_string, relationName, 0);
	
	return relation;
}

void Attachment::addTransaction(Transaction* transaction)
{
	transaction->tra_next = att_transactions;
	att_transactions = transaction;
}

void Attachment::addConnection(InternalConnection* connection)
{
	connection->prior = lastConnection;
	
	if (firstConnection)
		lastConnection->next = connection;
	else
		firstConnection = connection;

	lastConnection = connection;
	connection->next = NULL;
}
