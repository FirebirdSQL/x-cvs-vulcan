/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		gpre_meta.h
 *	DESCRIPTION:	Prototype header file for gpre_meta.c
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

#ifndef GPRE_GPRE_META_H
#define GPRE_GPRE_META_H


extern GPRE_FLD MET_context_field(GPRE_CTX, const TEXT *);
extern BOOLEAN MET_database(DBB, BOOLEAN);
extern USHORT MET_domain_lookup(GPRE_REQ, GPRE_FLD, const TEXT *);
extern GPRE_FLD MET_field(GPRE_REL, const TEXT *);
extern GPRE_NOD MET_fields(GPRE_CTX);
extern void MET_fini(DBB);
extern const char *MET_generator(const TEXT *, DBB);
extern BOOLEAN MET_get_column_default(GPRE_REL, const TEXT *, TEXT *, USHORT);
extern BOOLEAN MET_get_domain_default(DBB, const TEXT *, TEXT *, USHORT);
extern USHORT MET_get_dtype(USHORT, USHORT, USHORT *);
extern LLS MET_get_primary_key(DBB, const TEXT *);
extern GPRE_PRC MET_get_procedure(DBB, const TEXT *, const TEXT *);
extern GPRE_REL MET_get_relation(DBB, const TEXT *, const TEXT *);
extern INTLSYM MET_get_text_subtype(SSHORT);
extern UDF MET_get_udf(DBB, const TEXT *);
extern GPRE_REL MET_get_view_relation(GPRE_REQ, const TEXT *, const TEXT *, USHORT);
extern IND MET_index(DBB, const TEXT *);
extern void MET_load_hash_table(DBB);
extern GPRE_FLD MET_make_field(const TEXT *, SSHORT, SSHORT, BOOLEAN);
extern IND MET_make_index(const TEXT *);
extern GPRE_REL MET_make_relation(const TEXT *);
extern BOOLEAN MET_type(GPRE_FLD, const TEXT *, SSHORT *);
extern BOOLEAN MET_trigger_exists(DBB, const TEXT *);


#endif /* GPRE_GPRE_META_H */
