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
#include "../jrd/ods.h"
#include "../jrd/req.h"

class Transaction;

USHORT	BTR_all(TDBB, jrd_rel*, index_desc**, index_desc**, str**, SLONG*);
void	BTR_create(TDBB, jrd_rel*, Transaction*, index_desc*, USHORT, sort_context*, SelectivityList&);
void	BTR_delete_index(TDBB, win*, USHORT);
//USHORT	BTR_delete_node(TDBB, btr*, USHORT);
bool	BTR_description(DBB dbb, JRD_REL, irt*, index_desc*, SSHORT);
void	BTR_evaluate(tdbb*, IndexRetrieval*, sbm**);
UCHAR*	BTR_find_leaf(btr*, temporary_key*, UCHAR*, USHORT*, int, bool);
btr*	BTR_find_page(tdbb*, IndexRetrieval*, win*, index_desc*, temporary_key*, temporary_key*, bool);
void	BTR_insert(tdbb*, win*, index_insertion*);
enum idx_e	BTR_key(tdbb*, jrd_rel*, rec*, index_desc*, temporary_key*, idx_null_state*);
USHORT	BTR_key_length(TDBB tdbb, jrd_rel*, index_desc*);
UCHAR*	BTR_last_node(btr*, jrd_exp*, struct btx**);
btr*	BTR_left_handoff(tdbb*, win*, btr*, SSHORT);
USHORT	BTR_lookup(TDBB, jrd_rel*, USHORT, index_desc*);
void	BTR_make_key(tdbb*, USHORT, jrd_nod**, index_desc*, temporary_key*, USHORT);
bool	BTR_next_index(TDBB, jrd_rel*, Transaction*, index_desc*, win*);
void	BTR_remove(tdbb*, win*, index_insertion*);
void	BTR_reserve_slot(TDBB, jrd_rel*, Transaction*, index_desc*);
void	BTR_selectivity(TDBB, jrd_rel*, USHORT, SelectivityList&);

#endif // JRD_BTR_PROTO_H
