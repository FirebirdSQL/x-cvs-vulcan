/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		inter_proto.h
 *	DESCRIPTION:	Prototype Header file for interface.cpp
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

#ifndef REMOTE_INTER_PROTO_H
#define REMOTE_INTER_PROTO_H

class RTransaction;
class RDatabase;
class RRequest;
class RStatement;
class RBlob;

struct bid;

ISC_STATUS	REM_attach_database(ISC_STATUS*, const SCHAR*, RDatabase**,
	SSHORT, const UCHAR*, const TEXT*, ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration);
ISC_STATUS	REM_attach_service(ISC_STATUS *, USHORT, TEXT *, RDatabase **, USHORT, SCHAR *);
ISC_STATUS	REM_blob_info(ISC_STATUS*, RBlob**, SSHORT, const UCHAR*,SSHORT, UCHAR*);
ISC_STATUS	REM_cancel_blob(ISC_STATUS *, RBlob **);
ISC_STATUS	REM_cancel_events(ISC_STATUS *, RDatabase **, SLONG *);
ISC_STATUS	REM_close_blob(ISC_STATUS *, RBlob **);
ISC_STATUS	REM_commit_transaction(ISC_STATUS *, RTransaction **);
ISC_STATUS	REM_commit_retaining(ISC_STATUS *, RTransaction **);
ISC_STATUS	REM_compile_request(ISC_STATUS*, RDatabase**, RRequest**,
	USHORT, const UCHAR*);
ISC_STATUS	REM_create_blob2(ISC_STATUS*, RDatabase**, RTransaction**,
	RBlob**, bid*, USHORT, const UCHAR*);
ISC_STATUS	REM_create_database(ISC_STATUS*, const TEXT* orgName, const TEXT* translatedName, RDatabase**,
							    SSHORT dpbLength, const UCHAR* dpb, 
							    SSHORT databaseType, 
							    ConfObject* databaseConfiguration,
								ConfObject* providerConfiguration);
ISC_STATUS	REM_database_info(ISC_STATUS*, RDatabase**, SSHORT, const UCHAR*,
	SSHORT, UCHAR*);
ISC_STATUS	REM_ddl(ISC_STATUS*, RDatabase**, RTransaction**,
	USHORT, const UCHAR*);
ISC_STATUS	REM_detach_database(ISC_STATUS *, RDatabase **);
ISC_STATUS	REM_detach_service(ISC_STATUS *, RDatabase **);
ISC_STATUS	REM_drop_database(ISC_STATUS *, RDatabase **);
ISC_STATUS	REM_allocate_statement(ISC_STATUS *, RDatabase **, RStatement **);
ISC_STATUS	REM_execute(ISC_STATUS *, RTransaction **, RStatement **, USHORT, UCHAR *, USHORT, USHORT, UCHAR *);
ISC_STATUS	REM_execute2(ISC_STATUS *, RTransaction **, RStatement **, USHORT, UCHAR *, USHORT, USHORT, UCHAR *, USHORT, UCHAR *, USHORT, USHORT, UCHAR *);
ISC_STATUS	REM_execute_immediate(ISC_STATUS*, RDatabase**, RTransaction**,
	USHORT, const TEXT*, USHORT, USHORT, const UCHAR*, USHORT, USHORT, UCHAR*);
ISC_STATUS	REM_execute_immediate2(ISC_STATUS*, RDatabase**, RTransaction**,
	USHORT, const TEXT*, USHORT, USHORT, const UCHAR*, USHORT, USHORT, UCHAR*,
	USHORT, UCHAR*, USHORT, USHORT, UCHAR*);
ISC_STATUS	REM_fetch(ISC_STATUS*, RStatement**, USHORT, const UCHAR*, USHORT,
	USHORT, UCHAR*);
ISC_STATUS	REM_free_statement(ISC_STATUS *, RStatement **, USHORT);
ISC_STATUS	REM_insert(ISC_STATUS *, RStatement **, USHORT, UCHAR *, USHORT, USHORT, UCHAR *);
ISC_STATUS	REM_prepare(ISC_STATUS *, RTransaction **, RStatement **, USHORT, const TEXT *sql, USHORT, USHORT, const UCHAR *, USHORT, UCHAR *);
ISC_STATUS	REM_set_cursor_name(ISC_STATUS*, RStatement**, const TEXT*, USHORT);
ISC_STATUS	REM_sql_info(ISC_STATUS*, RStatement**, SSHORT, const UCHAR*,
	SSHORT, UCHAR*);
ISC_STATUS	REM_get_segment(ISC_STATUS *, RBlob **, int *, USHORT, UCHAR *);
ISC_STATUS	REM_get_slice(ISC_STATUS*, RDatabase**, RTransaction**, bid*, USHORT,
	const UCHAR*, USHORT, const UCHAR*, SLONG, UCHAR*, SLONG*);
ISC_STATUS	REM_open_blob2(ISC_STATUS*, RDatabase**, RTransaction**,
	RBlob**, bid*, USHORT, const UCHAR*);
ISC_STATUS	REM_prepare_transaction(ISC_STATUS *, RTransaction **, USHORT, UCHAR *);
ISC_STATUS	REM_put_segment(ISC_STATUS*, RBlob**, USHORT, const UCHAR*);
ISC_STATUS	REM_put_slice(ISC_STATUS*, RDatabase**, RTransaction**, bid*, USHORT,
	const UCHAR*, USHORT, const UCHAR*, SLONG, UCHAR*);
ISC_STATUS	REM_que_events(ISC_STATUS*, RDatabase**, SLONG*, SSHORT,
	const UCHAR*, FPTR_EVENT_CALLBACK, void*);
ISC_STATUS	REM_query_service(ISC_STATUS *, RDatabase **, USHORT, SCHAR *, USHORT, SCHAR *, USHORT, SCHAR *);
ISC_STATUS	REM_receive(ISC_STATUS *, RRequest **, USHORT, USHORT, UCHAR *, SSHORT);
ISC_STATUS	REM_reconnect_transaction(ISC_STATUS*, RDatabase**, RTransaction**,
	USHORT, const UCHAR*);
ISC_STATUS	REM_release_request(ISC_STATUS *, RRequest **);
ISC_STATUS	REM_request_info(ISC_STATUS*, RRequest**, SSHORT, SSHORT,
	const UCHAR*, SSHORT, UCHAR*);
ISC_STATUS	REM_rollback_transaction(ISC_STATUS *, RTransaction **);
ISC_STATUS	REM_seek_blob(ISC_STATUS *, RBlob **, SSHORT, SLONG, SLONG *);
ISC_STATUS	REM_send(ISC_STATUS *, RRequest **, USHORT, USHORT, const UCHAR *, SSHORT);
ISC_STATUS	REM_start_and_send(ISC_STATUS *, RRequest **, RTransaction **, USHORT, USHORT, const UCHAR *, SSHORT);
ISC_STATUS	REM_start_request(ISC_STATUS *, RRequest **, RTransaction **, USHORT);
ISC_STATUS	REM_start_transaction(ISC_STATUS *, RTransaction **, SSHORT, RDatabase **, SSHORT, UCHAR *);
ISC_STATUS	REM_transact_request(ISC_STATUS*, RDatabase**, RTransaction**,
	USHORT, const UCHAR*, USHORT, UCHAR*, USHORT, UCHAR*);
ISC_STATUS	REM_transaction_info(ISC_STATUS*, RTransaction**, SSHORT,
	const UCHAR*, SSHORT, UCHAR*);
ISC_STATUS	REM_unwind_request(ISC_STATUS *, RRequest **, USHORT);

ISC_STATUS	REM_rollback_retaining(ISC_STATUS *, RTransaction* *);
ISC_STATUS	REM_service_attach(ISC_STATUS*, USHORT, const TEXT*, RDatabase**, USHORT,
	const SCHAR*);
ISC_STATUS	REM_service_detach(ISC_STATUS *, RDatabase* *);
ISC_STATUS	REM_service_query(ISC_STATUS*, RDatabase**, ULONG*, USHORT, const SCHAR*,
									  USHORT, const SCHAR*, USHORT, SCHAR*);
ISC_STATUS	REM_service_start(ISC_STATUS*, RDatabase**, ULONG*, USHORT, const SCHAR*);



#endif	/* REMOTE_INTER_PROTO_H */

