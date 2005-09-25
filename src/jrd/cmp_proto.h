/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cmp_proto.h
 *	DESCRIPTION:	Prototype header file for cmp.cpp
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
 */

#ifndef JRD_CMP_PROTO_H
#define JRD_CMP_PROTO_H

#include "CompilerScratch.h"
//#include "req.h"
//#include "Resource.h"

// Note: Due to bad planning, the enum "rsc_s" is defined in req.h, which must be include
// before this file.

class Request;
class jrd_nod;
class Transaction;
class Procedure;
class Relation;
class Resource;
class Format;
class IndexLock;
//class Resource;

struct thread_db;

bool		CMP_clone_is_active(const Request*);
jrd_nod*	CMP_clone_node(thread_db*, CompilerScratch*, jrd_nod*);
Request*	CMP_clone_request(thread_db*, Request*, USHORT, bool);
Request*	CMP_compile2(thread_db*, const UCHAR*, USHORT);
CompilerScratch::csb_repeat* CMP_csb_element(CompilerScratch*, USHORT);
void		CMP_expunge_transaction(Transaction*);
void		CMP_decrement_prc_use_count(thread_db*, Procedure*);
Request*	CMP_find_request(thread_db*, USHORT, USHORT);
void		CMP_fini(thread_db*);
Format*		CMP_format(thread_db*, CompilerScratch*, USHORT);
void		CMP_get_desc(thread_db*, CompilerScratch*, jrd_nod*, dsc*);
IndexLock*	CMP_get_index_lock(thread_db* tdbb, Relation*, USHORT);
SLONG		CMP_impure(CompilerScratch*, USHORT);
Request*	CMP_make_request(thread_db*, CompilerScratch*);
void		CMP_post_access(thread_db*, CompilerScratch*, const TEXT*, SLONG,
					 const TEXT*, const TEXT*, USHORT, const TEXT*,
					 const TEXT*);
//void		CMP_post_resource(thread_db*, Resource**, blk*, enum Resource::rsc_s, USHORT);
//void		CMP_release_resource(Resource**, enum Resource::rsc_s, USHORT);
void		CMP_release(thread_db*, Request*);
void		CMP_shutdown_database(thread_db*);

#endif // JRD_CMP_PROTO_H
