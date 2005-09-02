/*
 *	PROGRAM:	JRD Backup and Restore program  
 *	MODULE:		burp_proto.h
 *	DESCRIPTION:	Prototype header file for burp.cpp
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

#ifndef BURP_BURP_PROTO_H
#define BURP_BURP_PROTO_H

#include <stdarg.h>

#ifdef SUPERSERVER
//int BURP_main(Service* service);
THREAD_ENTRY_DECLARE BURP_main(THREAD_ENTRY_PARAM arg);
#endif

void	BURP_abort(void);
void	BURP_error(USHORT, bool, USHORT, const void*, USHORT, const void*,
						USHORT, const void*, USHORT, const void*, USHORT, const void*);
void	BURP_error(USHORT, bool, const void*, const void*, const void*, const void*, const void*);
void	BURP_print_status(ISC_STATUS*);
void	BURP_error_redirect(ISC_STATUS*, USHORT, const void*, const void*);

//void	BURP_msg_partial(USHORT, const void*, const void*, const void*, const void*, const void*);
//void	BURP_msg_put(USHORT, const void*, const void*, const void*, const void*, const void*);
//void	BURP_msg_get(USHORT, TEXT*, const void*, const void*, const void*, const void*, const void*);
//void	BURP_print(USHORT, const void*, const void*, const void*, const void*, const void*);
//void	BURP_verbose(USHORT, const void*, const void*, const void*, const void*, const void*);

void	BURP_msg_partial(USHORT, ...);
void	BURP_msg_put(USHORT, ...);
void	BURP_msg_get(USHORT, TEXT*, ...);
void	BURP_print(USHORT, ...);
void	BURP_print(va_list stuff, USHORT);
void	BURP_verbose(USHORT, ...);

void	BURP_output_version(void*, const TEXT*);
void	BURP_print_warning(ISC_STATUS*);

#endif	//  BURP_BURP_PROTO_H

