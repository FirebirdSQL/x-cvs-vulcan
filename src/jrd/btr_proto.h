/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		btr_proto.h
 *	DESCRIPTION:	Prototype header file for btr.cpp
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

#ifndef _JRD_BTR_PROTO_H_
#define _JRD_BTR_PROTO_H_

#include "../jrd/btr.h"
#include "../jrd/err_proto.h"
//#include "../jrd/ods.h"
//#include "../jrd/req.h"

class Transaction;
struct IndexDescAlloc;
struct index_desc;
struct index_root_page;
struct btree_page;
struct SortContext;

USHORT	BTR_all(thread_db*, Relation*, IndexDescAlloc**);
void	BTR_create(thread_db*, Relation*, Transaction*, index_desc*, USHORT, SortContext*, SelectivityList&);
void	BTR_delete_index(thread_db*, win*, USHORT);
//USHORT	BTR_delete_node(thread_db*, btree_page*, USHORT);
bool	BTR_description(thread_db*, Relation*, index_root_page*, index_desc*, SSHORT);
void	BTR_evaluate(thread_db*, IndexRetrieval*, RecordBitmap**);
UCHAR*	BTR_find_leaf(btree_page*, temporary_key*, UCHAR*, USHORT*, bool, bool);
btree_page*	BTR_find_page(thread_db*, IndexRetrieval*, win*, index_desc*, temporary_key*, temporary_key*, bool);
void	BTR_insert(thread_db*, win*, index_insertion*);
enum idx_e	BTR_key(thread_db*, Relation*, Record*, index_desc*, temporary_key*, idx_null_state*);
USHORT	BTR_key_length(thread_db* tdbb, Relation*, index_desc*);
UCHAR*	BTR_last_node(btree_page*, exp_index_buf*, struct btree_exp**);
btree_page*	BTR_left_handoff(thread_db*, win*, btree_page*, SSHORT);
USHORT	BTR_lookup(thread_db*, Relation*, USHORT, index_desc*);
void	BTR_make_key(thread_db*, USHORT, jrd_nod**, index_desc*, temporary_key*, bool);
void	BTR_make_null_key(thread_db*, index_desc*, temporary_key*);
bool	BTR_next_index(thread_db*, Relation*, Transaction*, index_desc*, win*);
void	BTR_remove(thread_db*, win*, index_insertion*);
void	BTR_reserve_slot(thread_db*, Relation*, Transaction*, index_desc*);
void	BTR_selectivity(thread_db*, Relation*, USHORT, SelectivityList&);
void	BTR_complement_key(temporary_key*);

#endif // JRD_BTR_PROTO_H
