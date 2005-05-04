/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		met_proto.h
 *	DESCRIPTION:	Prototype header file for met.cpp
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

#ifndef JRD_MET_PROTO_H
#define JRD_MET_PROTO_H

#include "../jrd/exe.h"
#include "../jrd/jrn.h"
#include "../jrd/blob_filter.h"
#include "../jrd/Trigger.h"

class Transaction;
class Request;
class Procedure;
class ProcManager;
class Field;
class jrd_nod;
class Format;
class CompilerScratch;
class logfiles;
class sdw;
class Triggers;

struct bid;
struct dsc;
struct index_desc;

void		MET_activate_shadow(thread_db*);
ULONG		MET_align(Database*, const dsc*, USHORT);
void		MET_change_fields(thread_db*, Transaction*, dsc*);
//fmt*		MET_current(thread_db* tdbb, Relation *);
void		MET_delete_dependencies(thread_db*, TEXT*, USHORT);
void		MET_delete_shadow(thread_db*, USHORT);
void		MET_error(const TEXT*, ...);
SCHAR*		MET_exact_name(TEXT*);
Format*		MET_format(thread_db*, Relation *, USHORT);
BOOLEAN		MET_get_char_subtype(thread_db*, SSHORT*, const TEXT*, USHORT);
jrd_nod*	MET_get_dependencies(thread_db*, Relation*, TEXT*,
								CompilerScratch*, bid*, Request**,
								CompilerScratch**, const TEXT*, USHORT, USHORT);
Field*		MET_get_field(Relation*, USHORT);
void		MET_get_shadow_files(thread_db*, bool);
//ULONG		MET_get_walinfo(thread_db* tdbb, logfiles **, ULONG *, logfiles **);
void		MET_load_trigger(thread_db*, Relation*, const TEXT*, Triggers**);
void		MET_lookup_cnstrt_for_index(thread_db*, TEXT* constraint, const TEXT* index_name);
void		MET_lookup_cnstrt_for_trigger(thread_db*, TEXT*, TEXT*, const TEXT*);
void		MET_lookup_exception(thread_db*, SLONG, /* INOUT */ TEXT*, /* INOUT */ TEXT*);
SLONG		MET_lookup_exception_number(thread_db*, const TEXT*);
int			MET_lookup_field(thread_db*, Relation*, const TEXT*, const TEXT*);
BLF			MET_lookup_filter(thread_db*, SSHORT, SSHORT);
SLONG		MET_lookup_generator(thread_db*, const TEXT*);
void		MET_lookup_generator_id(thread_db*, SLONG, TEXT *);
void		MET_lookup_index(thread_db*, TEXT*, const TEXT*, USHORT);
SLONG		MET_lookup_index_name(thread_db*, const TEXT*, SLONG*, SSHORT*);
int			MET_lookup_partner(thread_db*, Relation*, index_desc*, const TEXT*);
Procedure*	MET_lookup_procedure(thread_db*, const TEXT*);
//Procedure*	MET_lookup_procedure_id(thread_db*, SSHORT, BOOLEAN, BOOLEAN, USHORT);
Relation*	MET_lookup_relation(thread_db*, const char*);
void		MET_getTypeInformation(thread_db*, Relation* relation);
Relation*	MET_lookup_relation_id(thread_db*, SLONG, BOOLEAN);
jrd_nod*	MET_parse_blob(thread_db*, Relation*, bid*, CompilerScratch**, Request **, BOOLEAN, BOOLEAN);
void		MET_parse_sys_trigger(thread_db*, Relation*);
int			MET_post_existence(thread_db*, Relation*);
void		MET_prepare(thread_db*, Transaction*, USHORT, const UCHAR*);
//Procedure*	MET_procedure(thread_db* tdbb, int, BOOLEAN, USHORT);
//Relation*	MET_relation(thread_db* tdbb, USHORT);
BOOLEAN		MET_relation_owns_trigger (thread_db*, const TEXT*, const TEXT*);
BOOLEAN		MET_relation_default_class (thread_db*, const TEXT*, const TEXT*);
void		MET_release_existence(Relation*);
void		MET_release_triggers(thread_db*, Triggers**);

#ifdef DEV_BUILD
void		MET_verify_cache(thread_db* tdbb);
#endif

BOOLEAN		MET_clear_cache(thread_db* tdbb, Procedure*);
BOOLEAN		MET_procedure_in_use(thread_db* tdbb, Procedure*);
void		MET_remove_procedure(thread_db* tdbb, Procedure*);
void		MET_revoke(thread_db* tdbb, Transaction *, TEXT *, TEXT *, TEXT *);
TEXT*		MET_save_name(thread_db* tdbb, const TEXT*);
void		MET_scan_relation(thread_db* tdbb, Relation *);
const TEXT* MET_trigger_msg(thread_db* tdbb, const TEXT*, USHORT);
void		MET_update_shadow(thread_db* tdbb, sdw *, USHORT);
void		MET_update_transaction(thread_db* tdbb, Transaction*, const bool);
void		MET_update_partners(thread_db* tdbb);
int			MET_resolve_charset_and_collation (thread_db* tdbb, const TEXT* charset, const TEXT* collation);

int			MET_get_primary_key (thread_db* tdbb, Relation *relation, int maxFields, Field **fields);

#endif // JRD_MET_PROTO_H

