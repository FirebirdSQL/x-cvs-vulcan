/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		evl_proto.h
 *	DESCRIPTION:	Prototype header file for evl.cpp
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

#ifndef JRD_EVL_PROTO_H
#define JRD_EVL_PROTO_H

#include "../jrd/intl_classes.h"

class Record;

dsc*		EVL_assign_to(thread_db*, jrd_nod*);
RecordBitmap**	EVL_bitmap(thread_db*, jrd_nod*);
bool		EVL_boolean(thread_db*, jrd_nod*);
dsc*		EVL_expr(thread_db*, jrd_nod*);
bool		EVL_field(Relation*, Record*, USHORT, DSC*);
USHORT		EVL_group(thread_db*, RecordSource*, jrd_nod*, USHORT);
USHORT		EVL_mb_contains(thread_db*, TextType, const UCHAR*, USHORT, const UCHAR*, USHORT);
USHORT		EVL_mb_like(thread_db*, TextType, const UCHAR*, SSHORT, const UCHAR*, SSHORT, USHORT);
USHORT		EVL_mb_matches(thread_db*, TextType, const UCHAR*, SSHORT, const UCHAR*, SSHORT);
USHORT		EVL_mb_sleuth_check(thread_db*, TextType, USHORT, const UCHAR*, USHORT,
								const UCHAR*,USHORT);
USHORT		EVL_mb_sleuth_merge(thread_db*, TextType, const UCHAR*, USHORT, const UCHAR*,
								USHORT, UCHAR*, USHORT);
void		EVL_make_value(thread_db*, const dsc*, impure_value*);
USHORT		EVL_nc_contains(thread_db*, TextType, const UCHAR*, USHORT, const UCHAR*, USHORT);
USHORT		EVL_nc_like(thread_db*, TextType, const UCHAR*, SSHORT, const UCHAR*, SSHORT, USHORT);
USHORT		EVL_nc_matches(thread_db*, TextType, const UCHAR*, SSHORT, const UCHAR*, SSHORT);
USHORT		EVL_nc_sleuth_check(thread_db*, TextType, USHORT, const UCHAR*, USHORT,
								const UCHAR*, USHORT);
USHORT		EVL_nc_sleuth_merge(thread_db*, TextType, const UCHAR*, USHORT, const UCHAR*,
								USHORT, UCHAR*, USHORT);
// CVC: evl.cpp declares abstract type UCS2_CHAR* instead of USHORT*
USHORT		EVL_wc_contains(thread_db*, TextType, const USHORT*, USHORT, const USHORT*, USHORT);
USHORT		EVL_wc_like(thread_db*, TextType, const USHORT*, SSHORT, const USHORT*, SSHORT, USHORT);
USHORT		EVL_wc_matches(thread_db*, TextType, const USHORT*, SSHORT, const USHORT*, SSHORT);
USHORT		EVL_wc_sleuth_check(thread_db*, TextType, USHORT, const USHORT*, USHORT,
								const USHORT*, USHORT);
USHORT		EVL_wc_sleuth_merge(thread_db*, TextType, const USHORT*, USHORT, const USHORT*,
								USHORT, USHORT*, USHORT);

#endif // JRD_EVL_PROTO_H

