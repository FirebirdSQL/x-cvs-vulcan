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
class fmt;
class CompilerScratch;
class logfiles;
class sdw;
class Triggers;

struct dsc;
struct index_desc;

void		MET_activate_shadow(thread_db* tdbb);
ULONG		MET_align(const dsc*, USHORT);
void		MET_change_fields(thread_db* tdbb, Transaction *, dsc *);
//fmt*		MET_current(thread_db* tdbb, Relation *);
void		MET_delete_dependencies(thread_db* tdbb, TEXT *, USHORT);
void		MET_delete_shadow(thread_db* tdbb, USHORT);
void		MET_error(const TEXT*, ...);
SCHAR*		MET_exact_name(TEXT*);
fmt*		MET_format(thread_db* tdbb, Relation *, USHORT);
BOOLEAN		MET_get_char_subtype(thread_db* tdbb, SSHORT*, const TEXT*, USHORT);
jrd_nod*	MET_get_dependencies(thread_db* tdbb, Relation*, TEXT*,
								CompilerScratch*, SLONG[2], Request**,
								CompilerScratch**, const TEXT*, USHORT);
Field*		MET_get_field(Relation *, USHORT);
void		MET_get_shadow_files(thread_db* tdbb, bool);
//ULONG		MET_get_walinfo(thread_db* tdbb, logfiles **, ULONG *, logfiles **);
void		MET_load_trigger(thread_db* tdbb, Relation*, const TEXT*, Triggers**);
void		MET_lookup_cnstrt_for_index(thread_db* tdbb, TEXT* constraint, const TEXT* index_name);
void		MET_lookup_cnstrt_for_trigger(thread_db* tdbb, TEXT*, TEXT*, const TEXT*);
void		MET_lookup_exception(thread_db* tdbb, SLONG, /* INOUT */ TEXT*, /* INOUT */ TEXT*);
SLONG		MET_lookup_exception_number(thread_db* tdbb, const TEXT*);
int			MET_lookup_field(thread_db* tdbb, Relation*, const TEXT*, const TEXT*);
BLF			MET_lookup_filter(thread_db* tdbb, SSHORT, SSHORT);
SLONG		MET_lookup_generator(thread_db* tdbb, const TEXT*);
void		MET_lookup_generator_id(thread_db* tdbb, SLONG, TEXT *);
void		MET_lookup_index(thread_db* tdbb, TEXT*, const TEXT*, USHORT);
SLONG		MET_lookup_index_name(thread_db* tdbb, const TEXT*, SLONG*, SSHORT*);
int			MET_lookup_partner(thread_db* tdbb, Relation*, index_desc*, const TEXT*);
Procedure*	MET_lookup_procedure(thread_db* tdbb, const TEXT*);
//Procedure*	MET_lookup_procedure_id(thread_db* tdbb, SSHORT, BOOLEAN, BOOLEAN, USHORT);
Relation*	MET_lookup_relation(thread_db* tdbb, const char*);
void		MET_getTypeInformation(thread_db* tdbb, Relation *relation);
Relation*	MET_lookup_relation_id(thread_db* tdbb, SLONG, BOOLEAN);
jrd_nod*	MET_parse_blob(thread_db* tdbb, Relation *, SLONG[2], CompilerScratch**, Request **, BOOLEAN, BOOLEAN);
void		MET_parse_sys_trigger(thread_db* tdbb, Relation *);
int			MET_post_existence(thread_db* tdbb, Relation *);
void		MET_prepare(thread_db* tdbb, Transaction*, USHORT, const UCHAR*);
//Procedure*	MET_procedure(thread_db* tdbb, int, BOOLEAN, USHORT);
//Relation*	MET_relation(thread_db* tdbb, USHORT);
BOOLEAN		MET_relation_owns_trigger (thread_db* tdbb, const TEXT*, const TEXT*);
BOOLEAN		MET_relation_default_class (thread_db* tdbb, const TEXT*, const TEXT*);
void		MET_release_existence(Relation *);
void		MET_release_triggers(thread_db* tdbb, Triggers* *);

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

