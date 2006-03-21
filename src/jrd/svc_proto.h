/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		svc_proto.h
 *	DESCRIPTION:	Prototype header file for svc.cpp
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

#ifndef JRD_SVC_PROTO_H
#define JRD_SVC_PROTO_H

#include "thd.h"

CLASS(ConfObject);

class Service;

struct thread_db;

Service* SVC_attach(ConfObject* configuration, const TEXT*, USHORT, const UCHAR*);
void   SVC_cleanup(Service *);
void   SVC_detach(Service *);
void   SVC_fprintf(Service*, const SCHAR*, ...);
void   SVC_putc(Service*, const UCHAR);
void   SVC_query(Service*, USHORT, const UCHAR*, USHORT, const UCHAR*, USHORT, UCHAR*);
ISC_STATUS SVC_query2(Service*, thread_db*, USHORT, const UCHAR*, USHORT, const UCHAR*, USHORT, UCHAR*);
void  SVC_start(Service*, USHORT, const UCHAR*);
void   SVC_finish(Service*, USHORT);
//int   SVC_read_ib_log(Service*);
THREAD_ENTRY_DECLARE SVC_read_ib_log(THREAD_ENTRY_PARAM arg);
//const TEXT* SVC_err_string(const TEXT*, USHORT);
int SVC_output(Service*, const UCHAR*);
void SVC_post(ISC_STATUS status, ...);

#ifdef SERVER_SHUTDOWN
typedef void (*shutdown_fct_t) (ULONG);
void SVC_shutdown_init(shutdown_fct_t, ULONG);
#endif /* SERVER_SHUTDOWN */

#endif /* JRD_SVC_PROTO_H */

