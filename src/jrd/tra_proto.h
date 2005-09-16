/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tra_proto.h
 *	DESCRIPTION:	Prototype header file for tra.cpp
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

#ifndef JRD_TRA_PROTO_H
#define JRD_TRA_PROTO_H

#include "../jrd/req.h"

BOOLEAN	TRA_active_transactions(thread_db*, Database *);
void	TRA_cleanup(thread_db*);
void	TRA_commit(thread_db*, Transaction*, const bool);
void	TRA_extend_tip(thread_db*, ULONG, struct win *);
int		TRA_fetch_state(thread_db*, SLONG);
void	TRA_get_inventory(thread_db*, UCHAR *, ULONG, ULONG);
int		TRA_get_state(thread_db*, SLONG);

#ifdef SUPERSERVER_V2
void	TRA_header_write(thread_db*, Database *, SLONG);
#endif
void	TRA_init(thread_db*);
void	TRA_invalidate(Database *, ULONG);
void	TRA_link_transaction(thread_db*, Transaction *);
void	TRA_post_resources(thread_db*, Transaction *, class Resource*);
BOOLEAN TRA_precommited(thread_db*, SLONG, SLONG);
void	TRA_prepare(thread_db*, Transaction*, USHORT, const UCHAR*);
Transaction* TRA_reconnect(thread_db*, const UCHAR*, USHORT);
void	TRA_release_transaction(thread_db*, Transaction *);
void	TRA_rollback(thread_db*, Transaction*, const bool);
void	TRA_set_state(thread_db*, Transaction *, SLONG, SSHORT);
void	TRA_shutdown_attachment(thread_db*, Attachment *);
int		TRA_snapshot_state(thread_db*, Transaction *, SLONG);
Transaction* TRA_start(thread_db*, int, const UCHAR*);
int		TRA_state(UCHAR *, ULONG, ULONG);
int		TRA_sweep(thread_db*, Transaction *);
Lock*	TRA_transaction_lock(thread_db*, struct blk *);
int		TRA_wait(thread_db*, Transaction *, SLONG, USHORT);

#endif // JRD_TRA_PROTO_H

