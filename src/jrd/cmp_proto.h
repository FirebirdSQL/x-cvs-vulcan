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

// Note: Due to bad planning, the enum "rsc_s" is defined in req.h, which must be include
// before this file.

class Request;
class jrd_nod;
class Transaction;
class Procedure;
class Relation;
class Rsc;
class Csb;
struct tdbb;
class fmt;
class idl;

struct csb_repeat;

bool		CMP_clone_is_active(const Request*);
jrd_nod*	CMP_clone_node(tdbb*, Csb*, jrd_nod*);
Request*	CMP_clone_request(tdbb*, Request*, USHORT, bool);
Request*	CMP_compile2(tdbb*, const UCHAR*, USHORT);
csb_repeat* CMP_csb_element(Csb*, USHORT);
void		CMP_expunge_transaction(Transaction*);
void		CMP_decrement_prc_use_count(tdbb*, Procedure*);
Request*	CMP_find_request(tdbb*, USHORT, USHORT);
void		CMP_fini(tdbb*);
fmt*		CMP_format(tdbb*, Csb*, USHORT);
void		CMP_get_desc(tdbb*, Csb*, jrd_nod*, dsc*);
idl*		CMP_get_index_lock(tdbb *tdbb, Relation*, USHORT);
SLONG		CMP_impure(Csb*, USHORT);
Request*	CMP_make_request(tdbb*, Csb*);
void		CMP_post_access(tdbb*, Csb*, const TEXT*, SLONG,
					 const TEXT*, const TEXT*, USHORT, const TEXT*,
					 const TEXT*);
void		CMP_post_resource(tdbb*, Rsc**, blk*, enum rsc_s, USHORT);
void		CMP_release_resource(Rsc**, enum rsc_s, USHORT);
void		CMP_release(tdbb*, Request*);
void		CMP_shutdown_database(tdbb*);

#endif // JRD_CMP_PROTO_H
