/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		vio_proto.h
 *	DESCRIPTION:	Prototype header file for vio.c
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
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 */

#ifndef JRD_VIO_PROTO_H
#define JRD_VIO_PROTO_H

class sav;
class JrdMemory;
struct rec;

void	VIO_backout(TDBB, struct rpb *, Transaction *);
void	VIO_bump_count(TDBB, USHORT, Relation *, bool);
int		VIO_chase_record_version(TDBB, struct rpb *, class Rsb *,
									Transaction *, JrdMemoryPool*, BOOLEAN);
#ifdef PC_ENGINE
int		VIO_check_if_updated(TDBB, struct rpb *);
#endif

void	VIO_data(TDBB, struct rpb *, JrdMemoryPool*);
void	VIO_erase(TDBB, struct rpb *, Transaction *);

bool	VIO_garbage_collect(TDBB, struct rpb *, Transaction *);
rec*	VIO_gc_record(TDBB, Relation *);
int		VIO_get(TDBB, struct rpb *, class Rsb *, Transaction *, JrdMemoryPool*);
int		VIO_get_current(TDBB, struct rpb *, Transaction *, JrdMemoryPool*, USHORT);

#ifdef GARBAGE_THREAD
void	VIO_init(TDBB);
void	VIO_fini(TDBB);
#endif

void	VIO_merge_proc_sav_points(TDBB, Transaction *, sav **);
BOOLEAN	VIO_writelock(TDBB, struct rpb *, class Rsb *, Transaction *);
void	VIO_modify(TDBB, struct rpb *, struct rpb *, Transaction *);
BOOLEAN	VIO_next_record(TDBB, struct rpb *, class Rsb *, Transaction *,
							   JrdMemoryPool*, BOOLEAN, BOOLEAN);
struct rec*	VIO_record(TDBB, struct rpb *, struct fmt *, JrdMemoryPool *);
void	VIO_start_save_point(TDBB, Transaction *);
void	VIO_store(TDBB, struct rpb *, Transaction *);
BOOLEAN	VIO_sweep(TDBB, Transaction *);
void	VIO_verb_cleanup(TDBB, Transaction *);
SLONG	VIO_savepoint_large(sav *, SLONG);

#endif // JRD_VIO_PROTO_H

