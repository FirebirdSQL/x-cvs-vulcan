/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dfw_proto.h
 *	DESCRIPTION:	Prototype header file for dfw.cpp
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

#ifndef JRD_DFW_PROTO_H
#define JRD_DFW_PROTO_H

class Transaction;
class tdbb;

USHORT DFW_assign_index_type(thread_db* tdbb, dfw*, SSHORT, SSHORT);
void DFW_delete_deferred(Transaction*, SLONG);
void DFW_merge_work(Transaction*, SLONG, SLONG);
void DFW_perform_system_work(thread_db* tdbb);
void DFW_perform_work(thread_db* tdbb, Transaction*);
void DFW_perform_post_commit_work(thread_db* tdbb, Transaction*);
dfw* DFW_post_work(Transaction*, enum dfw_t, dsc*, USHORT);
void DFW_post_work_arg(Transaction*, dfw*, dsc*, USHORT);
void DFW_update_index(thread_db* tdbb, const TEXT*, USHORT, const SelectivityList&);

#endif // JRD_DFW_PROTO_H
