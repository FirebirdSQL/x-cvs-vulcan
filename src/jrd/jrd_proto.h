/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		jrd_proto.h
 *	DESCRIPTION:	Prototype header file for jrd.cpp
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


#ifndef JRD_JRD_PROTO_H
#define JRD_JRD_PROTO_H

class Attachment;
class Service;
class blb;
class Transaction;
class Request;
CLASS(ConfObject);


ISC_STATUS jrd8_attach_database(ISC_STATUS* statusVector, 
									  const TEXT* orgName, 
									  const TEXT* translatedName, 
									  Attachment** dbHandle, 
									  SSHORT dpb_length, 
									  UCHAR* dpb,
									  ConfObject* databaseConfiguration,
									  ConfObject* providerConfiguration);
ISC_STATUS jrd8_blob_info(ISC_STATUS*, blb**, SSHORT,
										const UCHAR*, SSHORT, UCHAR*);
ISC_STATUS jrd8_cancel_blob(ISC_STATUS *, blb **);
ISC_STATUS jrd8_cancel_events(ISC_STATUS *, Attachment **, SLONG *);
#ifdef CANCEL_OPERATION
#define CANCEL_disable	1
#define CANCEL_enable	2
#define CANCEL_raise	3
ISC_STATUS jrd8_cancel_operation(ISC_STATUS *, Attachment **,
											   USHORT);
#endif
ISC_STATUS jrd8_close_blob(ISC_STATUS *, blb **);
ISC_STATUS jrd8_commit_transaction(ISC_STATUS *, Transaction **);
ISC_STATUS jrd8_commit_retaining(ISC_STATUS *, Transaction **);
ISC_STATUS jrd8_compile_request(ISC_STATUS*, Attachment**,
											  Request**,
											  SSHORT, const UCHAR*);
ISC_STATUS jrd8_create_blob2(ISC_STATUS*, Attachment**,
										   Transaction**, blb**,
										   struct bid*, USHORT, const UCHAR*);
ISC_STATUS jrd8_create_database(ISC_STATUS*, 
								const TEXT* orgName,
								const TEXT* translatedName,
								Attachment**, USHORT,
								const UCHAR*, USHORT,
								ConfObject* databaseConfiguration,
								ConfObject* providerConfiguration);
ISC_STATUS jrd8_database_info(ISC_STATUS*, Attachment**, SSHORT,
											const UCHAR*, SSHORT, UCHAR*);
ISC_STATUS jrd8_ddl(ISC_STATUS*, Attachment**, Transaction**,
								  USHORT, const UCHAR*);
ISC_STATUS jrd8_detach_database(ISC_STATUS *, Attachment **);
ISC_STATUS jrd8_drop_database(ISC_STATUS *, Attachment **);
ISC_STATUS jrd8_get_segment(ISC_STATUS *, blb **, int *,
										  USHORT, UCHAR *);
ISC_STATUS jrd8_get_slice(ISC_STATUS*, Attachment**,
										Transaction**, SLONG*, USHORT,
										const UCHAR*, USHORT, const UCHAR*, SLONG,
										UCHAR*, SLONG*);
ISC_STATUS jrd8_open_blob2(ISC_STATUS*, Attachment**,
										 Transaction**, blb**,
										 struct bid*, USHORT, const UCHAR*);
ISC_STATUS jrd8_prepare_transaction(ISC_STATUS *, Transaction **,
												  USHORT, UCHAR *);
ISC_STATUS jrd8_put_segment(ISC_STATUS*, blb**, USHORT,
										  const UCHAR*);
ISC_STATUS jrd8_put_slice(ISC_STATUS*, Attachment**,
										Transaction**, SLONG*, USHORT,
										const UCHAR*, USHORT, const UCHAR*, SLONG,
										UCHAR*);
ISC_STATUS jrd8_que_events(ISC_STATUS*, Attachment**, SLONG*,
										 SSHORT, const UCHAR*,
										 FPTR_EVENT_CALLBACK, void*);
ISC_STATUS jrd8_receive(ISC_STATUS *, Request **, USHORT, USHORT,
									  UCHAR *, SSHORT);
ISC_STATUS jrd8_reconnect_transaction(ISC_STATUS*, Attachment**,
													Transaction**, SSHORT,
													const UCHAR*);
ISC_STATUS jrd8_release_request(ISC_STATUS *, Request **);
ISC_STATUS jrd8_request_info(ISC_STATUS*, Request**, SSHORT,
										   SSHORT, const UCHAR*, SSHORT, UCHAR*);
ISC_STATUS jrd8_rollback_transaction(ISC_STATUS *, Transaction **);
ISC_STATUS jrd8_rollback_retaining(ISC_STATUS *, Transaction **);
ISC_STATUS jrd8_seek_blob(ISC_STATUS *, blb **, SSHORT,
										SLONG, SLONG *);
ISC_STATUS jrd8_send(ISC_STATUS *, Request **, USHORT, USHORT,
								   const UCHAR *, SSHORT);
ISC_STATUS jrd8_service_attach(ISC_STATUS*, USHORT, const TEXT*,
											 Service**, USHORT, const TEXT*);
ISC_STATUS jrd8_service_detach(ISC_STATUS *, Service **);
ISC_STATUS jrd8_service_query(ISC_STATUS*, Service**, ULONG*,
											USHORT, const SCHAR*,
											USHORT, const SCHAR*,
											USHORT, SCHAR*);
ISC_STATUS jrd8_service_start(ISC_STATUS*, Service**, ULONG*,
											USHORT, const SCHAR*);
ISC_STATUS jrd8_start_and_send(ISC_STATUS *, Request **,
											 Transaction **, USHORT, USHORT,
											 const UCHAR *, SSHORT);
ISC_STATUS jrd8_start_request(ISC_STATUS *, Request **,
											Transaction **, SSHORT);
ISC_STATUS jrd8_start_multiple(ISC_STATUS *, Transaction **, USHORT,
											 struct teb *);
ISC_STATUS jrd8_start_transaction(ISC_STATUS *, Transaction **,
												SSHORT, ...);
ISC_STATUS jrd8_transaction_info(ISC_STATUS*, Transaction**,
											   SSHORT, const UCHAR*, SSHORT,
											   UCHAR*);
ISC_STATUS jrd8_transact_request(ISC_STATUS*, Attachment**,
											   Transaction**, USHORT, const SCHAR*,
											   USHORT, SCHAR*, USHORT,
											   SCHAR*);
ISC_STATUS jrd8_unwind_request(ISC_STATUS *, Request **, SSHORT);
ISC_STATUS jrd8_update_account_info(ISC_STATUS *, Attachment**, int apbLength, const UCHAR *apb);
ISC_STATUS jrd8_user_info(ISC_STATUS*, Attachment**, int dpbLength, const UCHAR *dpb,
						  int itemsLength, const UCHAR *items,
						  int bufferLength, UCHAR *buffer);

void jrd_vtof(const char*, char*, SSHORT);

#ifdef SERVER_SHUTDOWN
/* Defines for paramater 3 of JRD_num_attachments */
#define JRD_info_drivemask	1
#define JRD_info_dbnames	2

//TEXT*	JRD_num_attachments(TEXT* const, USHORT, USHORT, USHORT*, USHORT*);
//ULONG	JRD_shutdown_all();
#endif /* SERVER_SHUTDOWN */

//void	JRD_set_cache_default(ULONG *);
void	JRD_blocked(Attachment *, struct btb **);
//void	JRD_mutex_lock(struct mutx_t *);
//void	JRD_mutex_unlock(struct mutx_t *);
BOOLEAN	JRD_reschedule(struct tdbb*, SLONG, bool);
void	JRD_restore_context(void);
void	JRD_set_context(struct tdbb *);
void	JRD_unblock(struct btb **);
//void	JRD_wlck_lock(struct mutx_t *);
//void	JRD_wlck_unlock(struct mutx_t *);

#ifdef SUPERSERVER
void	JRD_print_all_counters(const TEXT*);
USHORT	JRD_getdir(TEXT*, USHORT);
#endif

#ifdef DEBUG_PROCS
void	JRD_print_procedure_info(TDBB, const char*);
#endif

#endif /* JRD_JRD_PROTO_H */

