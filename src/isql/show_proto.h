/*
 *	PROGRAM:	Interactive SQL utility
 *	MODULE:		show_proto.h
 *	DESCRIPTION:	Prototype header file for show.e
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

#ifndef _ISQL_SHOW_PROTO_H_
#define _ISQL_SHOW_PROTO_H_



extern BOOLEAN	SHOW_dbb_parameters (isc_handle, SCHAR *, SCHAR *, USHORT, USHORT);
extern int	SHOW_grants (const SCHAR *, const SCHAR *, USHORT);
extern int	SHOW_grants2 (const SCHAR *, const SCHAR *, USHORT, TEXT *);
extern void	SHOW_grant_roles (const SCHAR *, SSHORT *);
extern void	SHOW_grant_roles2 (const SCHAR *, SSHORT *, TEXT *);
extern void SHOW_print_metadata_text_blob(FILE *, GDS__QUAD *);
extern int SHOW_metadata(SCHAR **, SCHAR **);

#endif /* _ISQL_SHOW_PROTO_H_ */
