/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe_proto.h
 *	DESCRIPTION:	Prototype header file for exe.cpp
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

#ifndef JRD_EXE_PROTO_H
#define JRD_EXE_PROTO_H

class jrd_node;
struct thread_db;
class Request;
//class Rsb;
class Transaction;

void EXE_assignment(thread_db*, jrd_nod*);
Request* EXE_find_request(thread_db*, Request *, bool);
void EXE_receive(thread_db*, Request*, USHORT, USHORT, UCHAR*);
void EXE_send(thread_db*, Request *, USHORT, USHORT, const UCHAR *);
void EXE_start(thread_db*, Request *, Transaction *);
//void EXE_unwind(Request *);

#ifdef SCROLLABLE_CURSORS
void EXE_seek(thread_db*, Request *, USHORT, ULONG);
#endif

#ifdef PC_ENGINE
bool EXE_crack(thread_db*, RecordSource*, USHORT);
void EXE_mark_crack(thread_db*, RecordSource*, USHORT);
#endif


#endif /* JRD_EXE_PROTO_H */
