/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		par_proto.h
 *	DESCRIPTION:	Prototype header file for par.cpp
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

#ifndef JRD_PAR_PROTO_H
#define JRD_PAR_PROTO_H

struct jrd_nod*	PAR_blr(thread_db*, Relation*, const UCHAR*, class CompilerScratch*,
					class CompilerScratch**, Request**, BOOLEAN, USHORT);
int				PAR_desc(class CompilerScratch*, struct dsc*);
struct jrd_nod*	PAR_gen_field(thread_db*, USHORT, USHORT);
struct jrd_nod*	PAR_make_field(thread_db*, class CompilerScratch*, USHORT, const TEXT*);
struct jrd_nod*	PAR_make_list(thread_db*, struct lls*);
struct jrd_nod*	PAR_make_node(thread_db*, int);
class CompilerScratch*	PAR_parse(thread_db*, const UCHAR*, USHORT);
SLONG			PAR_symbol_to_gdscode(const char*);

#endif // JRD_PAR_PROTO_H

