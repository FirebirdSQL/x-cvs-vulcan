/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		remote.h
 *	DESCRIPTION:	Common descriptions
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef REMOTE_REMOTE_H
#define REMOTE_REMOTE_H

#include "../jrd/common.h"
#include <ibase.h>
#include "../remote/remote_def.h"
//#include "../jrd/thd_proto.h"
//#include "../jrd/y_ref.h"
#include "ConfObj.h"

/* Include some apollo include files for tasking */

#if !(defined VMS || defined WIN_NT)
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif /* !VMS || !WIN_NT */


// Uncomment this line if you need to trace module activity
//#define REMOTE_DEBUG

#ifdef REMOTE_DEBUG
DEFINE_TRACE_ROUTINE(remote_trace);
#define REMOTE_TRACE(args) remote_trace args
#else
#define REMOTE_TRACE(args) /* nothing */
#endif

#ifdef DEV_BUILD
/* Debug packet/XDR memory allocation */

/* Temporarily disabling DEBUG_XDR_MEMORY */
/* #define DEBUG_XDR_MEMORY	*/

#endif

#define BLOB_LENGTH		16384   

#define ALLOC(type)			ALLR_block (type, 0)
#define ALLOCV(type, count)	ALLR_block (type, count)


#include "../remote/protocol.h"

/* Block types */

#ifndef INCLUDE_FB_BLK
//#include "../include/old_fb_blk.h"
#endif

class Port;
class RDatabase;
class RTransaction;
class RRequest;
class RStatement;
class RBlob;
class REVent;

/* Block types */

#include "RDatabase.h"
typedef RDatabase rdb;
typedef RDatabase *RDB;

#define RDB_service	1		/* structure relates to a service */

#include "RTransaction.h"
typedef RTransaction rtr;
typedef RTransaction *RTR;

#include "RBlob.h"
typedef RBlob rbl;
typedef RBlob *RBL;

#include "REvent.h"
typedef REvent rvnt;
typedef REvent *RVNT;

/***
typedef struct vec
{
	struct blk	vec_header;
	ULONG		vec_count;
	struct blk*	vec_object[1];
} *VEC;

typedef struct vcl
{
	struct blk	vcl_header;
	ULONG		vcl_count;
	SLONG		vcl_long[1];
} *VCL;
***/

/* Random string block -- jack of all kludges */

/***
typedef struct str
{
	struct blk	str_header;
	USHORT		str_length;
	SCHAR		str_data[2];
} *STR;
***/

/* Include definition of descriptor */

#include "RFormat.h"
typedef RFormat *FMT;
typedef RFormat fmt;

#ifdef OBSOLETE
#include "../jrd/dsc.h"

//typedef vary* VARY;

typedef struct fmt
{
	struct blk	fmt_header;
	USHORT		fmt_length;
	USHORT		fmt_net_length;
	USHORT		fmt_count;
	USHORT		fmt_version;
	USHORT		fmt_flags;
	struct dsc	fmt_desc[1];
} *FMT;
#endif

#define FMT_has_P10_specific_datatypes	0x1	/* datatypes don't exist in P9 */

/* Windows declares a msg structure, so rename the structure 
   to avoid overlap problems. */

#include "RMessage.h"
typedef RMessage *REM_MSG;
typedef RMessage message;

#ifdef OBSOLETE
typedef struct message
{
	struct blk	msg_header;
	struct message *msg_next;	/* Next available message */
#ifdef SCROLLABLE_CURSORS
	struct message *msg_prior;	/* Next available message */
	ULONG	msg_absolute; 		/* Absolute record number in cursor result set */
#endif
	/* Please DO NOT re-arrange the order of following two fields.
	   This could result in alignment problems while trying to access
	   'msg_buffer' as a 'long', leading to "core" drops 
		Sriram - 04-Jun-97 */
	USHORT	msg_number;			/* Message number */
	UCHAR	*msg_address;		/* Address of message */
	UCHAR	msg_buffer[1];		/* Allocated message */
} *REM_MSG;
#endif

/* remote stored procedure request */

#include "RProcedure.h"
typedef RProcedure rpr;
typedef RProcedure *RPR;


#include "RRequest.h"
typedef RRequest rrq;
typedef RRequest *RRQ;


/* remote SQL request */

#include "RStatement.h"
typedef RStatement	rsr;
typedef RStatement	*RSR;

enum blk_t
{
	type_MIN = 0,
	type_vec,
	type_fmt,
	type_str,
	//type_port,
	type_msg,
	//type_rpr,
	type_rmtque,
	type_MAX
};


#include "../remote/xdr.h"


/* Generalized port definition. */


#ifndef WIN_NT
#define HANDLE	int
#endif  /* WIN_NT */


//////////////////////////////////////////////////////////////////
// fwd. decl.
class Port;
struct p_cnct;

class port_interface
{
public:
	virtual int		accept_(Port* pPort, p_cnct* pConnection) = 0;
};

#include "Port.h"
//typedef Port *PORT;


/* Misc declarations */

//#include "../remote/allr_proto.h"
//#include "../jrd/thd.h"

/* Thread specific remote database block */

/***
typedef struct trdb
{
	struct thdd	trdb_thd_data;
	RDatabase*	trdb_database;
	ISC_STATUS*	trdb_status_vector;
} *TRDB;
***/

/***
#ifdef GET_THREAD_DATA
#undef GET_THREAD_DATA
#endif

#define GET_THREAD_DATA		((TRDB) THD_get_specific(THDD_TYPE_TRDB))
***/

/* Queuing structure for Client batch fetches */

typedef struct rmtque
{
	//struct blk			rmtque_header;	/* Memory allocator header */
	struct rmtque*		rmtque_next;	/* Next entry in queue */
	void*				rmtque_parm;/* What request has response in queue */
	rrq_repeat*		rmtque_message;/* What message is pending */
	RDatabase*			rmtque_rdb;	/* What database has pending msg */

	/* Fn that receives queued entry */
	bool	(*rmtque_function) (Port*,
								struct rmtque*,
								ISC_STATUS*,
								USHORT);
} *RMTQUE;

#endif /* REMOTE_REMOTE_H */

