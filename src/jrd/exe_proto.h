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
struct tdbb;
class Request;
class Rsb;
class Transaction;

void EXE_assignment(tdbb*, jrd_nod*);
Request* EXE_find_request(tdbb*, Request *, bool);
void EXE_receive(tdbb*, Request*, USHORT, USHORT, UCHAR*);
void EXE_send(tdbb*, Request *, USHORT, USHORT, const UCHAR *);
void EXE_start(tdbb*, Request *, Transaction *);
void EXE_unwind(tdbb*, Request *);
#ifdef SCROLLABLE_CURSORS
void EXE_seek(tdbb*, Request *, USHORT, ULONG);
#endif

#ifdef PC_ENGINE
bool EXE_crack(tdbb*, Rsb*, USHORT);
void EXE_mark_crack(tdbb*, Rsb*, USHORT);
#endif


#endif /* JRD_EXE_PROTO_H */
