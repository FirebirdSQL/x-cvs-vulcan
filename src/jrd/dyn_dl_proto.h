/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		dyn_dl_proto.h
 *	DESCRIPTION:	Prototype Header file for dyn_del.epp
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

#ifndef JRD_DYN_DL_PROTO_H
#define JRD_DYN_DL_PROTO_H

void	DYN_delete_constraint(thread_db* tdbb, GBL, const UCHAR**, const TEXT*);
void	DYN_delete_dimensions(thread_db* tdbb,GBL, const UCHAR**, const TEXT*, TEXT*);
void	DYN_delete_exception(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_filter(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_function(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_generator(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_global_field(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_index(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_local_field(thread_db* tdbb, GBL, const UCHAR**, const TEXT*, TEXT*);
void	DYN_delete_parameter(thread_db* tdbb, GBL, const UCHAR**, TEXT*);
void	DYN_delete_procedure(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_relation(thread_db* tdbb, GBL, const UCHAR**, const TEXT*);
void	DYN_delete_role(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_security_class(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_shadow(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_trigger(thread_db* tdbb, GBL, const UCHAR**);
void	DYN_delete_trigger_msg(thread_db* tdbb, GBL, const UCHAR**, TEXT*);

#endif // JRD_DYN_DL_PROTO_H

