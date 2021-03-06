/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		utl_proto.h
 *	DESCRIPTION:	Prototype header file for utl.cpp
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

#ifndef JRD_UTL_PROTO_H
#define JRD_UTL_PROTO_H

#ifndef INCLUDE_FB_TYPES_H
#include "../include/fb_types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int		API_ROUTINE gds__blob_size(isc_handle*, SLONG *, SLONG *, SLONG *);
void	API_ROUTINE_VARARG isc_expand_dpb(UCHAR**, SSHORT*, ...);
int		API_ROUTINE isc_modify_dpb(SCHAR**, SSHORT*, USHORT, const SCHAR*, SSHORT);
int		API_ROUTINE gds__edit(const TEXT*, USHORT);
ISC_LONG API_ROUTINE_VARARG isc_event_block(SCHAR**, SCHAR**, USHORT, ...);
USHORT	API_ROUTINE isc_event_block_a(SCHAR**, SCHAR**, USHORT, TEXT**);
void	API_ROUTINE isc_event_block_s(SCHAR**, SCHAR**, USHORT, TEXT**, USHORT*);

void	API_ROUTINE isc_event_counts(ISC_ULONG*, SSHORT, SCHAR*, const SCHAR*);
void	API_ROUTINE isc_get_client_version(SCHAR *);
int		API_ROUTINE isc_get_client_major_version();
int		API_ROUTINE isc_get_client_minor_version();
void	API_ROUTINE gds__map_blobs(int*, int*);
void	API_ROUTINE isc_set_debug(int);
void	API_ROUTINE isc_set_login(const UCHAR**, SSHORT*);
BOOLEAN	API_ROUTINE isc_set_path(const TEXT*, USHORT, TEXT*);
void	API_ROUTINE isc_set_single_user(const UCHAR**, SSHORT*, const TEXT*);
int		API_ROUTINE isc_version(isc_handle*, FPTR_VERSION_CALLBACK, void*);
void	API_ROUTINE isc_format_implementation(USHORT, USHORT, TEXT *,
												  USHORT, USHORT, TEXT *);
U_IPTR	API_ROUTINE isc_baddress(SCHAR*);
void	API_ROUTINE isc_baddress_s(const SCHAR*, U_IPTR*);
int		API_ROUTINE BLOB_close(struct bstream *);
int		API_ROUTINE blob__display(SLONG*, isc_handle*, isc_handle*, const TEXT*,
									 const SSHORT*);
int		API_ROUTINE BLOB_display(ISC_QUAD*, isc_handle, isc_handle, const TEXT*);
int		API_ROUTINE blob__dump(SLONG*, isc_handle*, isc_handle*, const TEXT*,
								  const SSHORT*);
int		API_ROUTINE BLOB_dump(ISC_QUAD*, isc_handle, isc_handle, const SCHAR*);
int		API_ROUTINE blob__edit(SLONG*, isc_handle*, isc_handle*, const TEXT*,
								  const SSHORT*);
int		API_ROUTINE BLOB_edit(ISC_QUAD*, isc_handle, isc_handle, const SCHAR*);
int		API_ROUTINE BLOB_get(struct bstream*);
int		API_ROUTINE blob__load(SLONG*, isc_handle*, isc_handle*, const TEXT*,
								  const SSHORT*);
int		API_ROUTINE BLOB_load(ISC_QUAD*, isc_handle, isc_handle, const TEXT*);
int		API_ROUTINE BLOB_text_dump(ISC_QUAD*, isc_handle, isc_handle, const SCHAR*);
int		API_ROUTINE BLOB_text_load(ISC_QUAD*, isc_handle, isc_handle, const TEXT*);
struct	bstream* API_ROUTINE Bopen(ISC_QUAD*, isc_handle, isc_handle, const SCHAR*);
struct  bstream* API_ROUTINE BLOB_open(isc_handle, SCHAR*, int);
int		API_ROUTINE BLOB_put(SCHAR, struct bstream*);

#ifdef VMS
ISC_STATUS API_ROUTINE gds__attach_database_d(ISC_STATUS*,
												 struct dsc$descriptor_s*,
												 int, SSHORT, const SCHAR*,
												 SSHORT);
void	API_ROUTINE gds__wake_init(void);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif // JRD_UTL_PROTO_H

