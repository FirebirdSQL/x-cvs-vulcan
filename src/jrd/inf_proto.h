/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		inf_proto.h
 *	DESCRIPTION:	Prototype header file for inf.cpp
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

#ifndef JRD_INF_PROTO_H
#define JRD_INF_PROTO_H

struct thdd;
struct thread_db;

int		INF_blob_info(const struct blb*, const UCHAR*, const SSHORT,
						UCHAR*, const SSHORT);
USHORT	INF_convert(SLONG, UCHAR*);
int		INF_database_info(thread_db* tdbb, const UCHAR*, const SSHORT, UCHAR*, const SSHORT);
UCHAR*	INF_put_item(UCHAR, USHORT, const void*, UCHAR*, const UCHAR*);
UCHAR* INF_put_item(UCHAR item, const char *text, UCHAR* ptr, const UCHAR* end);
int		INF_request_info(thread_db* tdbb, Request*, const UCHAR*, const SSHORT,
						UCHAR*, const SSHORT);
int		INF_transaction_info(const Transaction*, const UCHAR*, const SSHORT,
						UCHAR*, const SSHORT);

#endif // JRD_INF_PROTO_H

