/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Relation.h
 *	DESCRIPTION:	Relation (should be Table) class declaration
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
 * December 27, 2003	Created by James A. Starkey
 *
 */

// Relation.h: interface for the Relation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RELATION_H__E9A7F451_19FA_468E_8900_7B1CEA851EF4__INCLUDED_)
#define AFX_RELATION_H__E9A7F451_19FA_468E_8900_7B1CEA851EF4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"
#include "SyncObject.h"
#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "../include/fb_vector.h"
#include "SIVector.h"
#include "SVector.h"
#include "Interlock.h"

const USHORT REL_scanned				= 1;		/* Field expressions scanned (or being scanned) */
const USHORT REL_system					= 2;
const USHORT REL_deleted				= 4;		/* Relation known gonzo */
const USHORT REL_get_dependencies		= 8;			/* New relation needs dependencies during scan */
const USHORT REL_force_scan				= 16;		/* system relation has been updated since ODS change, force a scan */
const USHORT REL_check_existence		= 32;		/* Existence lock released pending drop of relation */
const USHORT REL_blocking				= 64;		/* Blocking someone from dropping relation */
const USHORT REL_sys_triggers			= 128;		/* The relation has system triggers to compile */
const USHORT REL_sql_relation			= 256;		/* Relation defined as sql table */
const USHORT REL_check_partners			= 512;		/* Rescan primary dependencies and foreign references */
const USHORT REL_being_scanned			= 1024;		/* relation scan in progress */
const USHORT REL_sys_trigs_being_loaded	= 2048;		/* System triggers being loaded */
const USHORT REL_deleting				= 4096;		/* relation delete in progress */
const USHORT REL_has_type_info			= 8192;		/* relation has field type information */

class Trigger;
class Triggers;
class Field;
class Database;
class Procedure;
class Connection;
class Transaction;
class vec;
class IndexLock;
class idb;
class fmt;
class RecordSelExpr;
class vcl;
class vcx;
class ext;
class sbm;
class lck;
class dsql_rel;

struct thread_db;

typedef firebird::vector<Trigger> TriggerVector;

struct prim {
	vec* prim_reference_ids;
	vec* prim_relations;
	vec* prim_indexes;
	prim(void);
};

typedef prim *PRIM;

/* Foreign references to other relations' primary/unique keys */

struct frgn {
	vec* frgn_reference_ids;
	vec* frgn_relations;
	vec* frgn_indexes;
	frgn(void);
};

typedef frgn *FRGN;

class Relation //: public pool_alloc<type_rel>
{
public:
	Relation(Database *dbb, int id);
	virtual ~Relation();

	Database		*rel_database;
	Relation		*rel_next;
	int				rel_id;
	USHORT			rel_flags;
	USHORT			rel_current_fmt;		/* Current format number */
	UCHAR			rel_length;				/* length of ascii relation name */
	fmt				*rel_current_format;	/* Current record format */
	JString			rel_name;				/* pointer to ascii relation name */
	//vec*			rel_formats;			/* Known record formats */
	SVector<fmt*>	rel_formats;			/* Known record formats */
	JString			rel_owner_name;			/* pointer to ascii owner */
	SIVector<SLONG>	rel_pages;				/* vector of pointer page numbers */
	SVector<Field*>	rel_fields;				/* vector of field blocks */

	RecordSelExpr*	rel_view_rse;			/* view record select expression */
	vcx				*rel_view_contexts;		/* linked list of view contexts */

	TEXT			*rel_security_name;		/* pointer to security class name for relation */
	ext				*rel_file;				/* external file name */
	SLONG			rel_index_root;			/* index root page number */
	SLONG			rel_data_pages;			/* count of relation data pages */

	vec*			rel_gc_rec;				/* vector of records for garbage collection */

//#ifdef GARBAGE_THREAD
	sbm			*rel_gc_bitmap;			/* garbage collect bitmap of data page sequences */
//#endif

	USHORT		rel_slot_space;			/* lowest pointer page with slot space */
	USHORT		rel_data_space;			/* lowest pointer page with data page space */
	//USHORT		rel_use_count;		/* requests compiled with relation */
	INTERLOCK_TYPE	useCount;
	USHORT		rel_sweep_count;		/* sweep and/or garbage collector threads active */
	SSHORT		rel_scan_count;			/* concurrent sequential scan count */
	int			rel_dbkey_length;		/* length of dbkey (duh) */
	lck			*rel_existence_lock;	/* existence lock, if any */
	lck			*rel_interest_lock;		/* interest lock to ensure compatibility of relation and record locks */
	lck			*rel_record_locking;	/* lock to start record locking on relation */

	ULONG		rel_explicit_locks;		/* count of records explicitly locked in relation */
	ULONG		rel_read_locks;			/* count of records read locked in relation (implicit or explicit) */
	ULONG		rel_write_locks;		/* count of records write locked in relation (implicit or explicit) */
	ULONG		rel_lock_total;			/* count of records locked since database first attached */

	IndexLock*		rel_index_locks;	/* index existence locks */
	idb *			rel_index_blocks;	/* index blocks for caching index info */
	Triggers		*rel_pre_erase; 	/* Pre-operation erase trigger */
	Triggers		*rel_post_erase;	/* Post-operation erase trigger */
	Triggers		*rel_pre_modify;	/* Pre-operation modify trigger */
	Triggers		*rel_post_modify;	/* Post-operation modify trigger */
	Triggers		*rel_pre_store;		/* Pre-operation store trigger */
	Triggers		*rel_post_store;	/* Post-operation store trigger */
	prim			rel_primary_dpnds;	/* foreign dependencies on this relation's primary key */
	frgn			rel_foreign_refs;	/* foreign references to other relations' primary keys */
	dsql_rel		*dsqlRelation;
	
	SyncObject		syncObject;
	SyncObject		syncGarbageCollection;
	SyncObject		syncTriggers;
	SyncObject		syncFormats;
	Field			*junk;
	
	void scan(thread_db* tdbb);
	void getTypeInformation(thread_db* tdbb);
	int getPrimaryKey(thread_db* tdbb, int maxFields, Field** fields);
	Field* findField(thread_db* tdbb, const char* fieldName);
	Field* getField(thread_db* tdbb, const char* fieldName);
	static int blockingAst(void* astObject);
	int blockingAst(void);
	void createExistenceLock(thread_db* tdbb);
	void fetchFields(Connection* connection);
	void fetchFields(Transaction* transaction);
	void dropField(thread_db* tdbb, const char* field);
	Field* addField(int id, const char* name);
	int incrementUseCount(void);
	int decrementUseCount(void);
	Field* findField(int id);
	fmt* getFormat(thread_db* tdbb, int formatVersion);
	fmt* getCurrentFormat(thread_db* tdbb);
	void setFormat(fmt* format);
	void scanRelation(thread_db* tdbb, int csb_flags);
};

#endif // !defined(AFX_RELATION_H__E9A7F451_19FA_468E_8900_7B1CEA851EF4__INCLUDED_)
