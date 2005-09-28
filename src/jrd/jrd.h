/*
 *      PROGRAM:        JRD access method
 *      MODULE:         jrd.h
 *      DESCRIPTION:    Common descriptions
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
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#ifndef JRD_JRD_H
#define JRD_JRD_H

#include "../jrd/gdsassert.h"
#include "../jrd/common.h"
#include "../jrd/dsc.h"
#include "../jrd/all.h"
//#include "../jrd/nbak.h"

/***
#if defined(UNIX) && defined(SUPERSERVER)
#include <setjmp.h>
#endif
***/

#include "../include/fb_vector.h"

class JrdMemoryPool;
class Relation;
class Database;
class Field;
class Request;
class Procedure;

#ifdef DEV_BUILD
//#define DEBUG                   if (debug) DBG_supervisor(debug);
#define DEBUG                   
#define VIO_DEBUG				/* remove this for production build */
#define WALW_DEBUG				/* remove this for production build */
#else /* PROD */
#define DEBUG
#undef VIO_DEBUG
#undef WALW_DEBUG
#endif

#define BUGCHECK(number)        ERR_bugcheck (number)
#define CORRUPT(number)         ERR_corrupt (number)
#define IBERROR(number)         ERR_error (number)


//#define BLKCHK(blk, type)       if (MemoryPool::blk_type(blk) != (USHORT) (type)) BUGCHECK (147)
#define BLKCHK(blk, type) 

/* DEV_BLKCHK is used for internal consistency checking - where
 * the performance hit in production build isn't desired.
 * (eg: scatter this everywhere)
 *
 * This causes me a problem DEV_BLKCHK fails when the data seems valid
 * After talking to John this could be because the memory is from the local
 * stack rather than the heap.  However I found to continue I needed to 
 * turn it off by dfining the macro to be empty.  But In thinking about
 * it I think that it would be more helful for a mode where these extra 
 * DEV checks just gave warnings rather than being fatal.
 * MOD 29-July-2002
 *
 */
 
#ifdef DEV_BUILD
#define DEV_BLKCHK(blk,type)
//#define DEV_BLKCHK(blk,type)    if (blk) {BLKCHK (blk, type);}
#else
#define DEV_BLKCHK(blk,type)	/* nothing */
#endif


/* Thread data block / IPC related data blocks */

#include "../jrd/thd.h"
#include "../jrd/isc.h"

/* definition of block types for data allocation in JRD */
#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"


/* the database block, the topmost block in the metadata 
   cache for a database */

//#define HASH_SIZE 101


// fwd. decl.
class vec;
struct thread_db;

#include "Database.h"
typedef Database dbb;
typedef Database *DBB;


//
// Flags to indicate normal internal requests vs. dyn internal requests
//
#define IRQ_REQUESTS            1
#define DYN_REQUESTS            2


//
// Errors during validation - will be returned on info calls
// CVC: It seems they will be better in a header for val.cpp that's not val.h
//
const int VAL_PAG_WRONG_TYPE			= 0;
const int VAL_PAG_CHECKSUM_ERR			= 1;
const int VAL_PAG_DOUBLE_ALLOC			= 2;
const int VAL_PAG_IN_USE				= 3;
const int VAL_PAG_ORPHAN				= 4;
const int VAL_BLOB_INCONSISTENT			= 5;
const int VAL_BLOB_CORRUPT				= 6;
const int VAL_BLOB_TRUNCATED			= 7;
const int VAL_REC_CHAIN_BROKEN			= 8;
const int VAL_DATA_PAGE_CONFUSED		= 9;
const int VAL_DATA_PAGE_LINE_ERR		= 10;
const int VAL_INDEX_PAGE_CORRUPT		= 11;
const int VAL_P_PAGE_LOST				= 12;
const int VAL_P_PAGE_INCONSISTENT		= 13;
const int VAL_REC_DAMAGED				= 14;
const int VAL_REC_BAD_TID				= 15;
const int VAL_REC_FRAGMENT_CORRUPT		= 16;
const int VAL_REC_WRONG_LENGTH			= 17;
const int VAL_INDEX_ROOT_MISSING		= 18;
const int VAL_TIP_LOST					= 19;
const int VAL_TIP_LOST_SEQUENCE			= 20;
const int VAL_TIP_CONFUSED				= 21;
const int VAL_REL_CHAIN_ORPHANS			= 22;
const int VAL_INDEX_MISSING_ROWS		= 23;
const int VAL_INDEX_ORPHAN_CHILD		= 24;
const int VAL_MAX_ERROR					= 25;



//
// the attachment block; one is created for each attachment to a database
//

//#include "Attachment.h"



#include "Trigger.h"

//typedef firebird::vector<Trigger*> trig_vec;
//typedef trig_vec* TRIG_VEC;

/* Relation block; one is created for each relation referenced
   in the database, though it is not really filled out until
   the relation is scanned */

//#include "Relation.h"


/* Field block, one for each field in a scanned relation */

//#include "Field.h"
class Field;
//typedef Field jrd_fld;
//typedef Field *JRD_FLD;

struct jrd_nod;

/* Index block to cache index information */

class IndexBlock : public pool_alloc<type_idb>
{
public:
	IndexBlock	*idb_next;
	jrd_nod*	idb_expression;			/* node tree for index expression */
	Request*	idb_expression_request;	/* request in which index expression is evaluated */
	dsc			idb_expression_desc;	/* descriptor for expression result */
	Lock*		idb_lock;				/* lock to synchronize changes to index */
	UCHAR		idb_id;
};



/* view context block to cache view aliases */

class ViewContext : public pool_alloc<type_vcx>
{
    public:
	class ViewContext* vcx_next;
	class str* vcx_context_name;
	class str* vcx_relation_name;
	USHORT vcx_context;
};


#include "JVector.h"
#include "old_vector.h"
#include "que.h"


/* symbol definitions */

#include "Symb.h"
//typedef Sym *SYM;



/* Random string block -- jack of all kludges */

class str : public pool_alloc_rpt<SCHAR, type_str>
{
public:
	USHORT str_length;
	UCHAR str_data[2];			/* one byte for ALLOC and one for the NULL */

	/***
	static bool extend(str*& s, size_t new_len)
		{
		fb_assert(s);
		MemoryPool* pPool = MemoryPool::blk_pool(s);
		fb_assert(pPool);
		
		if (!pPool) 
			return false;	// runtime safety

		// TMN: Note that this violates "common sense" and should be fixed.
		
		str* res = FB_NEW_RPT(*pPool, new_len + 1) str;
		res->str_length = new_len;
		memcpy(res->str_data, s->str_data, s->str_length + 1);
		str* old = s;
		s = res;
		delete old;
		return s != 0;
		}
	***/
};
typedef str *STR;


//
// Transaction element block
//

struct TransElement {
	Attachment** teb_database;
	int			 teb_tpb_length;
	UCHAR *teb_tpb;
};

//typedef teb TEB;

/* Blocking Thread Block */

class BlockingThread : public pool_alloc<type_btb>
{
    public:
	BlockingThread* btb_next;
	SLONG btb_thread_id;
};

/* Lock levels */

#define LCK_none        0
#define LCK_null        1
#define LCK_SR          2
#define LCK_PR          3
#define LCK_SW          4
#define LCK_PW          5
#define LCK_EX          6

#define LCK_read        LCK_PR
#define LCK_write       LCK_EX

#define LCK_WAIT        TRUE
#define LCK_NO_WAIT     FALSE

/* Lock query data aggregates */

#define LCK_MIN		1
#define LCK_MAX		2
#define LCK_CNT		3
#define LCK_SUM		4
#define LCK_AVG		5
#define LCK_ANY		6


/* Window block for loading cached pages into */
// CVC: Apparently, the only possible values are HEADER_PAGE==0 and LOG_PAGE==2
// and reside in ods.h, although I watched a place with 1 and others with members
// of a struct.

class Bdb;
struct pag;
struct exp_index_buf;

typedef struct win {
	SLONG		win_page;
	pag*		win_buffer;
	exp_index_buf* win_expanded_buffer;
	Bdb*		win_bdb;
	//class bdb*	win_bdb;
	SSHORT		win_scans;
	USHORT		win_flags;
	explicit win(SLONG wp) : win_page(wp), win_flags(0) {}
} WIN;

// This is a compilation artifact: I wanted to be sure I would pick all old "win"
// declarations at the top, so "win" was built with a mandatory argument in
// the constructor. This struct satisfies a single place with an array. The
// alternative would be to initialize 16 elements of the array with 16 calls
// to the constructor: win my_array[n] = {win(-1), ... (win-1)};
// When all places are changed, this class can disappear and win's constructor
// may get the default value of -1 to "wp".

struct win_for_array: public win
{
	win_for_array() : win(-1) {}
};


#define	WIN_large_scan		1	/* large sequential scan */
#define WIN_secondary		2	/* secondary stream */
#define	WIN_garbage_collector	4	/* garbage collector's window */
#define WIN_garbage_collect	8	/* scan left a page for garbage collector */


/* define used for journaling start transaction */

#define MOD_START_TRAN  100

#include "tdbb.h"


/* List of internal database handles */

typedef struct ihndl
{
	struct ihndl*	ihndl_next;
	void*			ihndl_object;
} *IHNDL;


/* Threading macros */

#ifdef GET_THREAD_DATA
#undef GET_THREAD_DATA
#endif

#ifdef V4_THREADING
#define PLATFORM_GET_THREAD_DATA ((thread_db*) THD_get_specific(THDD_TYPE_TDBB))
#endif

/* RITTER - changed HP10 to HPUX in the expression below */
#ifdef MULTI_THREAD
#if (defined SOLARIS_MT || defined WIN_NT || \
	defined HPUX || defined POSIX_THREADS || defined DARWIN || defined FREEBSD )
#define PLATFORM_GET_THREAD_DATA ((thread_db*) THD_get_specific(THDD_TYPE_TDBB))
#endif
#endif

#ifndef PLATFORM_GET_THREAD_DATA

//extern TDBB gdbb;

#define PLATFORM_GET_THREAD_DATA (gdbb)
#endif

/* Define GET_THREAD_DATA off the platform specific version.
 * If we're in DEV mode, also do consistancy checks on the
 * retrieved memory structure.  This was originally done to
 * track down cases of no "PUT_THREAD_DATA" on the NLM. 
 *
 * This allows for NULL thread data (which might be an error by itself)
 * If there is thread data, 
 * AND it is tagged as being a TDBB.
 * AND it has a non-NULL tdbb_database field, 
 * THEN we validate that the structure there is a database block.
 * Otherwise, we return what we got.
 * We can't always validate the database field, as during initialization
 * there is no tdbb_database set up.
 */

#ifdef DEV_BUILD_XXX
#define GET_THREAD_DATA (((PLATFORM_GET_THREAD_DATA) && \
                         (((thread_db*)(PLATFORM_GET_THREAD_DATA))->thdd_type == THDD_TYPE_TDBB) && \
			 (((thread_db*)(PLATFORM_GET_THREAD_DATA))->tdbb_database)) \
			 ? ((MemoryPool::blk_type(((thread_db*)(PLATFORM_GET_THREAD_DATA))->tdbb_database) == type_dbb) \
			    ? (PLATFORM_GET_THREAD_DATA) \
			    : (BUGCHECK (147), (PLATFORM_GET_THREAD_DATA))) \
			 : (PLATFORM_GET_THREAD_DATA))
#define CHECK_DBB(dbb)   fb_assert ((dbb) && (MemoryPool::blk_type(dbb) == type_dbb) && ((dbb)->dbb_permanent->verify_pool()))
#define CHECK_TDBB(tdbb) fb_assert ((tdbb) && \
	(((thread_db*)(tdbb))->thdd_type == THDD_TYPE_TDBB) && \
	((!(tdbb)->tdbb_database)||MemoryPool::blk_type((tdbb)->tdbb_database) == type_dbb))
#else
/* PROD_BUILD */
#define GET_THREAD_DATA (PLATFORM_GET_THREAD_DATA)
#define CHECK_TDBB(tdbb)		/* nothing */
#define CHECK_DBB(dbb)			/* nothing */
#endif

#define GET_DBB         (((thread_db*) (GET_THREAD_DATA))->tdbb_database)

/*-------------------------------------------------------------------------*
 * macros used to set tdbb and dbb pointers when there are not set already *
 *-------------------------------------------------------------------------*/

#define	SET_TDBB(tdbb)	if ((tdbb) == NULL) { (tdbb) = GET_THREAD_DATA; }; CHECK_TDBB (tdbb)
#define	SET_DBB(dbb)	if ((dbb)  == NULL)  { (dbb)  = GET_DBB; }; CHECK_DBB(dbb);

#ifdef V4_THREADING
#define V4_JRD_MUTEX_LOCK(mutx)         JRD_mutex_lock (mutx)
#define V4_JRD_MUTEX_UNLOCK(mutx)       JRD_mutex_unlock (mutx)
#define V4_JRD_RW_LOCK_LOCK(wlck,type)  JRD_wlck_lock (wlck, type)
#define V4_JRD_RW_LOCK_UNLOCK(wlck)     JRD_wlck_unlock (wlck)
// BRS. 03/23/2003
// Those empty defines was substituted with #ifdef V4_THREADING
//#else
//#define V4_JRD_MUTEX_LOCK(mutx)
//#define V4_JRD_MUTEX_UNLOCK(mutx)
//#define V4_JRD_RW_LOCK_LOCK(wlck,type)
//#define V4_JRD_RW_LOCK_UNLOCK(wlck)
#endif


/* global variables for engine */


#if !defined(REQUESTER)

extern int debug;
//extern IHNDL internal_db_handles;

#endif /* REQUESTER */


/* Define the xxx_THREAD_DATA macros.  These are needed in the whole 
   component, but they are defined differently for use in jrd.c (JRD_MAIN)
   Here we have a function which sets some flags, and then calls THD_put_specific
   so in this case we define the macro as calling that function. */
#ifndef JRD_MAIN

#ifdef __cplusplus

#define SET_THREAD_DATA		tdbb = &thd_context;\
				MOVE_CLEAR (tdbb, sizeof (*tdbb));\
				THD_put_specific (reinterpret_cast<struct thdd*>(tdbb), THDD_TYPE_TDBB);\
				tdbb->tdbb_thd_data.thdd_type = THDD_TYPE_TDBB

#else

#define SET_THREAD_DATA		tdbb = &thd_context;\
				MOVE_CLEAR (tdbb, sizeof (*tdbb));\
				THD_put_specific((struct thdd*)tdbb), THDD_TYPE_TDBB;\
				tdbb->tdbb_thd_data.thdd_type = THDD_TYPE_TDBB
#endif /* __cplusplus */

#define RESTORE_THREAD_DATA	THD_restore_specific(THDD_TYPE_TDBB)

#endif /* !JRD_MAIN */



#endif /* JRD_JRD_H */

