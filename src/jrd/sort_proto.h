/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		sort_proto.h
 *	DESCRIPTION:	Prototype header file for sort.cpp
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

#ifndef JRD_SORT_PROTO_H
#define JRD_SORT_PROTO_H

struct thread_db;
struct sort_work_file;
struct sort_context;
struct sort_key_def;

#ifdef SCROLLABLE_CURSORS
void	SORT_diddle_key(UCHAR *, sort_context*, bool);
void	SORT_get(thread_db*, sort_context*, ULONG**, RSE_GET_MODE);
#else
void	SORT_get(thread_db*, sort_context*, ULONG**);
#endif

ULONG	SORT_read_block(sort_work_file*, ULONG, BLOB_PTR*, ULONG);

void	SORT_error(sort_work_file*, const TEXT *, ISC_STATUS, int);
void	SORT_fini(sort_context*, Attachment *);
struct sort_context*	SORT_init(thread_db*, USHORT, USHORT, USHORT, const sort_key_def*,
						FPTR_REJECT_DUP_CALLBACK, void*, Attachment*, UINT64);
void	SORT_put(thread_db*, sort_context*, ULONG**);
void	SORT_shutdown(Attachment*);
bool	SORT_sort(thread_db*, sort_context*);
ULONG	SORT_write_block(sort_work_file*, ULONG, BLOB_PTR*, ULONG);

#endif // JRD_SORT_PROTO_H

