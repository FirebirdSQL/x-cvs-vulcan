/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		idx_proto.h
 *	DESCRIPTION:	Prototype header file for idx.cpp
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

#ifndef JRD_IDX_PROTO_H
#define JRD_IDX_PROTO_H

#include "../jrd/btr.h"
#include "../jrd/exe.h"
#include "../jrd/req.h"


void IDX_check_access(thread_db*, class CompilerScratch*, Relation* , Relation* ,
							 Field *);
void IDX_create_index(thread_db*, Relation*, struct index_desc*, const TEXT*,
					  USHORT*, Transaction*, SelectivityList&);
struct IndexBlock* IDX_create_index_block(thread_db*, Relation* , USHORT);
void IDX_delete_index(thread_db*, Relation* , USHORT);
void IDX_delete_indices(thread_db*, Relation* );
enum idx_e IDX_erase(thread_db*, struct record_param* , Transaction *, Relation* *,
							USHORT *);
void IDX_garbage_collect(thread_db*, struct record_param* , struct lls *,
								struct lls *);
enum idx_e IDX_modify(struct thread_db*, struct record_param* , struct record_param* ,
							 Transaction *, Relation* *, USHORT *);
enum idx_e IDX_modify_check_constraints(thread_db*, struct record_param* , struct record_param* ,
											  Transaction *, Relation* *,
											  USHORT *);
void IDX_statistics(thread_db*, Relation* , USHORT, SelectivityList&);
enum idx_e IDX_store(struct thread_db*, struct record_param* , Transaction *,
							Relation **, USHORT *);


#endif // JRD_IDX_PROTO_H

