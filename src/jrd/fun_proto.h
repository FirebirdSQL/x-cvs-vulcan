/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		fun_proto.h
 *	DESCRIPTION:	Prototype header file for fun.c
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

#ifndef JRD_FUN_PROTO_H
#define JRD_FUN_PROTO_H

class Function;
struct impure_value;

void		FUN_evaluate(thread_db* tdbb, Function*, jrd_nod*, impure_value*);
void		FUN_fini(thread_db*);
void		FUN_init(void);
Function*	FUN_lookup_function(thread_db* tdbb, const TEXT*, bool ShowAccessError);
Function*	FUN_resolve(thread_db* tdbb, class CompilerScratch*, Function*, jrd_nod*);

#endif // JRD_FUN_PROTO_H
