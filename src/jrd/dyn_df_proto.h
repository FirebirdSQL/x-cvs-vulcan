/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		dyn_df_proto.h
 *	DESCRIPTION:	Prototype Header file for dyn_def.epp
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

#ifndef JRD_DYN_DF_PROTO_H
#define JRD_DYN_DF_PROTO_H

void DYN_define_cache(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_constraint(TDBB tdbb, GBL, const UCHAR**, const TEXT*, TEXT*);
void DYN_define_dimension(TDBB tdbb, GBL, const UCHAR**, const TEXT*, TEXT*);
void DYN_define_exception(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_file(TDBB tdbb, GBL, const UCHAR**, SLONG, SLONG*, USHORT);
void DYN_define_filter(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_function(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_function_arg(TDBB tdbb, GBL, const UCHAR**, TEXT*);
void DYN_define_generator(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_global_field(TDBB tdbb, GBL, const UCHAR**, const TEXT*, TEXT*);
void DYN_define_index(TDBB tdbb, GBL, const UCHAR**, const TEXT*, UCHAR, TEXT*, TEXT*,
							 TEXT*, UCHAR *);
void DYN_define_local_field(TDBB tdbb, GBL, const UCHAR**, const TEXT*, TEXT*);
void DYN_define_log_file(TDBB tdbb, GBL, const UCHAR**, bool, bool);
void DYN_define_parameter(TDBB tdbb, GBL, const UCHAR**, TEXT*);
void DYN_define_procedure(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_relation(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_role(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_security_class(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_shadow(TDBB tdbb, GBL, const UCHAR**);
void DYN_define_sql_field(TDBB tdbb, GBL, const UCHAR**, const TEXT*, TEXT*);
void DYN_define_trigger(TDBB tdbb, GBL, const UCHAR**, const TEXT*, TEXT*, const bool);
void DYN_define_trigger_msg(TDBB tdbb, GBL, const UCHAR**, const TEXT*);
void DYN_define_view_relation(TDBB tdbb, GBL, const UCHAR**, const TEXT*);
void DYN_define_difference(TDBB tdbb, GBL, const UCHAR**);

#endif // JRD_DYN_DF_PROTO_H

