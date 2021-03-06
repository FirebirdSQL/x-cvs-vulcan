/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dpm_proto.h
 *	DESCRIPTION:	Prototype header file for dpm.cpp
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

#ifndef JRD_DPM_PROTO_H
#define JRD_DPM_PROTO_H

// fwd. decl.
class blb;
class RecordNumber;
class Relation;
class Format;

struct pag* DPM_allocate(thread_db*, struct win*);
void	DPM_backout(thread_db*, struct record_param*);
double	DPM_cardinality(thread_db*, Relation*, const Format*);
int		DPM_chain(thread_db*, struct record_param* , struct record_param*);
int		DPM_compress(thread_db*, struct data_page*);
void	DPM_create_relation(thread_db*, Relation*);
SLONG	DPM_data_pages(thread_db*, Relation*);
void	DPM_delete(thread_db*, struct record_param*, SLONG);
void	DPM_delete_relation(thread_db*, Relation*);
BOOLEAN	DPM_fetch(thread_db*, struct record_param*, USHORT);
SSHORT	DPM_fetch_back(thread_db*, struct record_param*, USHORT, SSHORT);
void	DPM_fetch_fragment(thread_db*, struct record_param*, USHORT);
SINT64	DPM_gen_id(thread_db*, SLONG, USHORT, SINT64);
int		DPM_get(thread_db*, struct record_param*, SSHORT);
ULONG	DPM_get_blob(thread_db*, blb*, RecordNumber, USHORT, SLONG);
BOOLEAN	DPM_next(thread_db*, struct record_param*, USHORT, BOOLEAN, BOOLEAN);
void	DPM_pages(thread_db*, SSHORT, int, ULONG, SLONG);
SLONG	DPM_prefetch_bitmap(struct thread_db*, Relation*, class sbm *, SLONG);
void	DPM_scan_pages(thread_db*);
void	DPM_store(thread_db*, struct record_param* , struct lls **, USHORT);
RecordNumber	DPM_store_blob(thread_db*, blb*, struct Record*);
void	DPM_rewrite_header(thread_db*, struct record_param* );
void	DPM_update(thread_db*, struct record_param* , struct lls **, Transaction*);

#endif /* JRD_DPM_PROTO_H */
