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

class Savepoint;
class JrdMemory;
struct rec;

void	VIO_backout(thread_db*, struct record_param* , Transaction *);
void	VIO_bump_count(thread_db*, USHORT, Relation *, bool);
int		VIO_chase_record_version(thread_db*, struct record_param* , class RecordSource *,
									Transaction *, JrdMemoryPool*, BOOLEAN);
#ifdef PC_ENGINE
int		VIO_check_if_updated(thread_db*, struct record_param* );
#endif

void	VIO_data(thread_db*, struct record_param*, JrdMemoryPool*);
void	VIO_erase(thread_db*, struct record_param*, Transaction*);

bool	VIO_garbage_collect(thread_db*, struct record_param*, Transaction*);
Record*	VIO_gc_record(thread_db*, Relation*);
int		VIO_get(thread_db*, struct record_param*, class RecordSource*, Transaction*, JrdMemoryPool*);
int		VIO_get_current(thread_db*, struct record_param*, Transaction*, JrdMemoryPool*, USHORT);

#ifdef GARBAGE_THREAD
void	VIO_init(thread_db*);
void	VIO_fini(thread_db*);
#endif

void	VIO_merge_proc_sav_points(thread_db*, Transaction*, Savepoint**);
BOOLEAN	VIO_writelock(thread_db*, struct record_param* , class RecordSource*, Transaction*);
void	VIO_modify(thread_db*, struct record_param* , struct record_param* , Transaction*);
BOOLEAN	VIO_next_record(thread_db*, struct record_param* , class RecordSource*, Transaction*,
							   JrdMemoryPool*, BOOLEAN, BOOLEAN);
struct Record*	VIO_record(thread_db*, struct record_param* , struct Format*, JrdMemoryPool*);
void	VIO_start_save_point(thread_db*, Transaction *);
void	VIO_store(thread_db*, struct record_param* , Transaction*);
BOOLEAN	VIO_sweep(thread_db*, Transaction*);
void	VIO_verb_cleanup(thread_db*, Transaction*);
SLONG	VIO_savepoint_large(Savepoint*, SLONG);

#endif // JRD_VIO_PROTO_H

