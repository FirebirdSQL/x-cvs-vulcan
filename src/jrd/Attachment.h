/* $Id$ */
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Attachment.h
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

// Attachment.h: interface for the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ATTACHMENT_H__79215D0A_E447_411D_A318_BD185E131E4F__INCLUDED_)
#define AFX_ATTACHMENT_H__79215D0A_E447_411D_A318_BD185E131E4F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../include/fb_vector.h"
#include "JString.h"
#include "SyncObject.h"
#include "UserData.h"

struct thread_db;

/* Attachment flags */

const ULONG ATT_no_cleanup			= 1;	// Don't expunge, purge, or garbage collect
const ULONG ATT_shutdown			= 2;	// attachment has been shutdown
const ULONG ATT_shutdown_notify		= 4;	// attachment has notified client of shutdown
const ULONG ATT_shutdown_manager	= 8;	// attachment requesting shutdown
const ULONG ATT_lck_init_done		= 16;	// LCK_init() called for the attachment
const ULONG ATT_exclusive			= 32;	// attachment wants exclusive database access
const ULONG ATT_attach_pending		= 64;	// Indicate attachment is only pending
const ULONG ATT_exclusive_pending	= 128;	// Indicate exclusive attachment pending
const ULONG ATT_gbak_attachment		= 256;	// Indicate GBAK attachment
const ULONG ATT_security_db			= 512;	// Indicates an implicit attachment to the security db

#ifdef GARBAGE_THREAD
const ULONG ATT_notify_gc			= 1024;	// Notify garbage collector to expunge, purge ..
const ULONG ATT_disable_notify_gc	= 2048;	// Temporarily perform own garbage collection
const ULONG ATT_garbage_collector	= 4096;	// I'm a garbage collector
#define ATT_NO_CLEANUP	(ATT_no_cleanup | ATT_notify_gc)
#else
#define ATT_NO_CLEANUP	ATT_no_cleanup
#endif

#ifdef CANCEL_OPERATION
const ULONG ATT_cancel_raise		= 8192;		// Cancel currently running operation
const ULONG ATT_cancel_disable		= 16384;	// Disable cancel operations
#endif

const ULONG ATT_gfix_attachment		= 32768;	// Indicate a GFIX attachment
const ULONG ATT_gstat_attachment	= 65536;	// Indicate a GSTAT attachment
const ULONG ATT_internal			= (1 << 17);	// Internal connection

//
// Database attachments
//
const int DBB_read_seq_count		= 0;
const int DBB_read_idx_count		= 1;
const int DBB_update_count			= 2;
const int DBB_insert_count			= 3;
const int DBB_delete_count			= 4;
const int DBB_backout_count			= 5;
const int DBB_purge_count			= 6;
const int DBB_expunge_count			= 7;
const int DBB_max_count				= 8;



class Database;
class Cursor;
class DStatement;
class Transaction;
class InternalConnection;
class Relation;
class vec;
class vcl;
class str;
class UserId;
class Request;
class Lock;
class SortContext;
class SecurityClass;
//class Bookmark;


class Attachment //: public pool_alloc<type_att>
{
public:
	Attachment(Database *database);
	virtual ~Attachment();
	Cursor*		allocateCursor(DStatement* statement, Transaction* transaction);
	DStatement* allocateStatement(void);
	InternalConnection* getUserConnection(Transaction* transaction);
	void closeConnection(InternalConnection* connection);

	Database*	att_database;			// Parent databasea block
	Attachment*	att_next;				// Next attachment to database
	Attachment*	att_blocking;			// Blocking attachment, if any
	Cursor		*cursors;				// Active cursors
	//UserId*		att_user;				// User identification
	Transaction*	att_transactions;	// Transactions belonging to attachment
	Transaction*	att_dbkey_trans;	// transaction to control db-key scope
	Request*	att_requests;			// Requests belonging to attachment
	SortContext*		att_active_sorts;	// Active sorts
	Lock*		att_id_lock;			// Attachment lock (if any)
	SLONG		att_attachment_id;		// Attachment ID
	SLONG		att_lock_owner_handle;	// Handle for the lock manager
	SLONG		att_event_session;		// Event session id, if any
	SecurityClass*		att_security_class;	// security class for database
	SecurityClass*		att_security_classes;	// security classes
	vcl*		att_counts[DBB_max_count];
	vec*		att_relation_locks;		// explicit persistent locks for relations
	Lock*		att_record_locks;		// explicit or implicit record locks taken out during attachment
	ULONG		att_flags;				// Flags describing the state of the attachment
	SSHORT		att_charset;			// user's charset specified in dpb
	str*		att_lc_messages;		// attachment's preference for message natural language
	Lock*		att_long_locks;			// outstanding two phased locks
	vec*		att_compatibility_table;	// hash table of compatible locks
	vcl*		att_val_errors;
	JString		att_working_directory;	// Current working directory is cached
	JString		att_filename;			// alias used to attach the database
	GDS_TIMESTAMP	att_timestamp;		// connection date and time

	int					userFlags;
	UserData			userData;
	InternalConnection	*firstConnection;
	InternalConnection	*lastConnection;
	
#ifdef SHARED_CACHE
	SyncObject	syncObject;
	SyncObject	syncLongLocks;
	SyncObject	syncRequests;
#endif

	bool isSoleAttachment(void);
	Cursor* findCursor(const char* name);
	void deleteCursor(Cursor* cursor);
	void endTransaction(Transaction* transaction);
	void updateAccountInfo(thread_db *tdbb, int apbLength, const UCHAR* apb);
	void authenticateUser(thread_db* tdbb, int dpbLength, const UCHAR* dpb);
	void shutdown(thread_db *tdbb);
	void addLongLock(Lock* lock);
	void removeLongLock(Lock* lock);
	Lock* findBlock(Lock* lock, int level);
	Relation* findRelation(thread_db* tdbb, int relationId, int csbFlags=0);
	Relation* getRelation(thread_db* tdbb, int relationId);
	Relation* findRelation(thread_db* tdbb, const char* relationName, int csbFlags=0);
	Relation* getRelation(thread_db* tdbb, const char* relationName);
	void addTransaction(Transaction* transaction);
	void addConnection(InternalConnection* connection);
};

#endif // !defined(AFX_ATTACHMENT_H__79215D0A_E447_411D_A318_BD185E131E4F__INCLUDED_)
