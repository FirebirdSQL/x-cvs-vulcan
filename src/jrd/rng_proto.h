/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		rng_proto.h
 *	DESCRIPTION:	Prototype header file for rng.c
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

#ifndef JRD_RNG_PROTO_H
#define JRD_RNG_PROTO_H

#ifdef PC_ENGINE
void RNG_add_page(TDBB tdbb, ULONG);
void RNG_add_record(TDBB tdbb, struct rpb *);
struct jrd_nod *RNG_add_relation(TDBB tdbb, struct jrd_nod *);
void RNG_add_uncommitted_record(TDBB tdbb, struct rpb *);
struct dsc *RNG_begin(TDBB tdbb, struct jrd_nod *, struct vlu *);
struct jrd_nod *RNG_delete(TDBB tdbb, struct jrd_nod *);
void RNG_delete_ranges(Request *);
struct jrd_nod *RNG_end(TDBB tdbb, struct jrd_nod *);
void RNG_release_locks(TDBB tdbb, struct rng *);
void RNG_release_ranges(Request *);
void RNG_shutdown_attachment(TDBB tdbb, Attachment *);
#endif

#endif // JRD_RNG_PROTO_H
