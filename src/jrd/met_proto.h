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
class Csb;
class logfiles;
class sdw;
class Triggers;

struct dsc;
struct index_desc;

void		MET_activate_shadow(tdbb* tdbb);
ULONG		MET_align(const dsc*, USHORT);
void		MET_change_fields(tdbb* tdbb, Transaction *, dsc *);
//fmt*		MET_current(tdbb* tdbb, Relation *);
void		MET_delete_dependencies(tdbb* tdbb, TEXT *, USHORT);
void		MET_delete_shadow(tdbb* tdbb, USHORT);
void		MET_error(const TEXT*, ...);
SCHAR*		MET_exact_name(TEXT*);
fmt*		MET_format(tdbb* tdbb, Relation *, USHORT);
BOOLEAN		MET_get_char_subtype(tdbb* tdbb, SSHORT*, const TEXT*, USHORT);
jrd_nod*	MET_get_dependencies(tdbb* tdbb, Relation*, TEXT*,
								Csb*, SLONG[2], Request**,
								Csb **, const TEXT*, USHORT);
Field*		MET_get_field(Relation *, USHORT);
void		MET_get_shadow_files(tdbb* tdbb, bool);
//ULONG		MET_get_walinfo(tdbb* tdbb, logfiles **, ULONG *, logfiles **);
void		MET_load_trigger(tdbb* tdbb, Relation*, const TEXT*, Triggers**);
void		MET_lookup_cnstrt_for_index(tdbb* tdbb, TEXT* constraint, const TEXT* index_name);
void		MET_lookup_cnstrt_for_trigger(tdbb* tdbb, TEXT*, TEXT*, const TEXT*);
void		MET_lookup_exception(tdbb* tdbb, SLONG, /* INOUT */ TEXT*, /* INOUT */ TEXT*);
SLONG		MET_lookup_exception_number(tdbb* tdbb, const TEXT*);
int			MET_lookup_field(tdbb* tdbb, Relation*, const TEXT*, const TEXT*);
BLF			MET_lookup_filter(tdbb* tdbb, SSHORT, SSHORT);
SLONG		MET_lookup_generator(tdbb* tdbb, const TEXT*);
void		MET_lookup_generator_id(tdbb* tdbb, SLONG, TEXT *);
void		MET_lookup_index(tdbb* tdbb, TEXT*, const TEXT*, USHORT);
SLONG		MET_lookup_index_name(tdbb* tdbb, const TEXT*, SLONG*, SSHORT*);
int			MET_lookup_partner(tdbb* tdbb, Relation*, index_desc*, const TEXT*);
Procedure*	MET_lookup_procedure(tdbb* tdbb, const TEXT*);
//Procedure*	MET_lookup_procedure_id(tdbb* tdbb, SSHORT, BOOLEAN, BOOLEAN, USHORT);
Relation*	MET_lookup_relation(tdbb* tdbb, const char*);
void		MET_getTypeInformation(tdbb* tdbb, Relation *relation);
Relation*	MET_lookup_relation_id(tdbb* tdbb, SLONG, BOOLEAN);
jrd_nod*	MET_parse_blob(tdbb* tdbb, Relation *, SLONG[2], Csb**, Request **, BOOLEAN, BOOLEAN);
void		MET_parse_sys_trigger(tdbb* tdbb, Relation *);
int			MET_post_existence(tdbb* tdbb, Relation *);
void		MET_prepare(tdbb* tdbb, Transaction*, USHORT, const UCHAR*);
//Procedure*	MET_procedure(tdbb* tdbb, int, BOOLEAN, USHORT);
//Relation*	MET_relation(tdbb* tdbb, USHORT);
BOOLEAN		MET_relation_owns_trigger (tdbb* tdbb, const TEXT*, const TEXT*);
BOOLEAN		MET_relation_default_class (tdbb* tdbb, const TEXT*, const TEXT*);
void		MET_release_existence(Relation *);
void		MET_release_triggers(tdbb* tdbb, Triggers* *);

#ifdef DEV_BUILD
void		MET_verify_cache(tdbb* tdbb);
#endif

BOOLEAN		MET_clear_cache(tdbb* tdbb, Procedure*);
BOOLEAN		MET_procedure_in_use(tdbb* tdbb, Procedure*);
void		MET_remove_procedure(tdbb* tdbb, Procedure*);
void		MET_revoke(tdbb* tdbb, Transaction *, TEXT *, TEXT *, TEXT *);
TEXT*		MET_save_name(tdbb* tdbb, const TEXT*);
void		MET_scan_relation(tdbb* tdbb, Relation *);
const TEXT* MET_trigger_msg(tdbb* tdbb, const TEXT*, USHORT);
void		MET_update_shadow(tdbb* tdbb, sdw *, USHORT);
void		MET_update_transaction(tdbb* tdbb, Transaction*, const bool);
void		MET_update_partners(tdbb *tdbb);
int			MET_resolve_charset_and_collation (tdbb* tdbb, const TEXT* charset, const TEXT* collation);

int			MET_get_primary_key (tdbb* tdbb, Relation *relation, int maxFields, Field **fields);

#endif // JRD_MET_PROTO_H

