/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pag_proto.h
 *	DESCRIPTION:	Prototype header file for pag.cpp
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

#ifndef JRD_PAG_PROTO_H
#define JRD_PAG_PROTO_H

struct thread_db;

int		PAG_add_clump(thread_db* tdbb, SLONG, USHORT, USHORT, const UCHAR*, USHORT, USHORT);
USHORT	PAG_add_file(thread_db* tdbb, TEXT *, SLONG);
int		PAG_add_header_entry(thread_db* tdbb, struct header_page*, USHORT, SSHORT, UCHAR *);
int		PAG_replace_entry_first(thread_db* tdbb, struct header_page*, USHORT, SSHORT, UCHAR *);
struct pag*	PAG_allocate(thread_db* tdbb, struct win *);
SLONG	PAG_attachment_id(thread_db* tdbb);
int		PAG_delete_clump_entry(thread_db* tdbb, SLONG, USHORT);
void	PAG_format_header(thread_db* tdbb);
void	PAG_format_log(thread_db* tdbb);
void	PAG_format_pip(thread_db* tdbb);
int		PAG_get_clump(thread_db* tdbb, SLONG, USHORT, USHORT *, UCHAR *);
void	PAG_header(thread_db* tdbb, const TEXT*);
void	PAG_init(thread_db* tdbb);
void	PAG_init2(thread_db* tdbb, USHORT);
SLONG	PAG_last_page(thread_db* tdbb);
void	PAG_modify_log(thread_db* tdbb, SLONG, SLONG);
void	PAG_release_page(thread_db* tdbb, SLONG, SLONG);
void	PAG_set_force_write(thread_db* tdbb, Database *, SSHORT);
void	PAG_set_no_reserve(thread_db* tdbb, Database *, USHORT);
void	PAG_set_db_readonly(thread_db* tdbb, Database *, SSHORT);
void	PAG_set_db_SQL_dialect(thread_db* tdbb, Database *, SSHORT);
void	PAG_set_page_buffers(thread_db* tdbb, ULONG);
void	PAG_sweep_interval(thread_db* tdbb, SLONG);

#endif // JRD_PAG_PROTO_H

