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
struct tdbb;

void	EXT_close(tdbb *tdbb, Rsb*);
void	EXT_erase(tdbb *tdbb, rpb*, int*);
ext*	EXT_file(tdbb *tdbb, jrd_rel*, const TEXT*, SLONG*);
void	EXT_fini(tdbb *tdbb, jrd_rel*);
int		EXT_get(tdbb *tdbb, Rsb*);
void	EXT_modify(tdbb *tdbb, rpb*, rpb*, int*);

#ifdef VMS
int	EXT_open(tdbb *tdbb, Rsb*);
#else
void	EXT_open(tdbb *tdbb, Rsb*);
#endif
Rsb*	EXT_optimize(tdbb *tdbb, Opt*, SSHORT, jrd_nod**);
void	EXT_ready(jrd_rel*);
void	EXT_store(DBB dbb, rpb*, int*);
void	EXT_trans_commit(Transaction*);
void	EXT_trans_prepare(Transaction*);
void	EXT_trans_rollback(Transaction*);
void	EXT_trans_start(Transaction*);

#endif // JRD_EXT_PROTO_H
