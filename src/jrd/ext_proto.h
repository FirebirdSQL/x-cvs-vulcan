/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ext_proto.h
 *	DESCRIPTION:	Prototype header file for ext.cpp & extvms.cpp
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

#ifndef JRD_EXT_PROTO_H
#define JRD_EXT_PROTO_H

class ext;
class Transaction;
struct thread_db;

void	EXT_close(thread_db* tdbb, RecordSource*);
void	EXT_erase(thread_db* tdbb, record_param*, int*);
ext*	EXT_file(thread_db* tdbb, Relation*, const TEXT*, SLONG*);
void	EXT_fini(thread_db* tdbb, Relation*);
int		EXT_get(thread_db* tdbb, RecordSource*);
void	EXT_modify(thread_db* tdbb, record_param*, record_param*, int*);

#ifdef VMS
int	EXT_open(thread_db* tdbb, RecordSource*);
#else
void	EXT_open(thread_db* tdbb, RecordSource*);
#endif
RecordSource*	EXT_optimize(thread_db* tdbb, OptimizerBlk*, SSHORT, jrd_nod**);
void	EXT_ready(Relation*);
void	EXT_store(DBB dbb, record_param*, int*);
void	EXT_trans_commit(Transaction*);
void	EXT_trans_prepare(Transaction*);
void	EXT_trans_rollback(Transaction*);
void	EXT_trans_start(Transaction*);

#endif // JRD_EXT_PROTO_H
