/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		make_proto.h
 *	DESCRIPTION:	Prototype Header file for make.cpp
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
 *
 * 2002-07-20 Arno Brinkman: Added MAKE_desc_from_list
 */

#ifndef DSQL_MAKE_PROTO_H
#define DSQL_MAKE_PROTO_H

#include "../dsql/sym.h"

class Database;
class CStatement;
class dsql_msg;
class dsql_str;
class dsql_ctx;
class dsql_fld;
class dslq_ddl;
class dsql_req;
class ProcParam;
class Stack;

dsql_nod*		MAKE_constant(tdbb *threadData, dsql_str*, dsql_constant_type);
dsql_nod*		MAKE_str_constant(tdbb *threadData, dsql_str*, SSHORT);
dsql_str*		MAKE_cstring(tdbb *threadData, const char*);
void			MAKE_desc(tdbb *threadData, dsc*, dsql_nod*, dsql_nod*);
void			MAKE_desc_from_field(tdbb *threadData, dsc*, const dsql_fld*);
void			MAKE_desc_from_list(tdbb *threadData, dsc*, dsql_nod*, dsql_nod*, const TEXT*);
dsql_nod*		MAKE_field(tdbb *threadData, dsql_ctx*, dsql_fld*, dsql_nod*);
dsql_nod*		MAKE_list(tdbb *threadData, Stack*);
dsql_nod*		MAKE_node(tdbb *threadData, enum nod_t, int);
//class par*	MAKE_parameter(CStatement* request, dsql_msg* , bool, bool, USHORT);
dsql_str*		MAKE_string(tdbb *threadData, const char* , int);
dsql_sym*		MAKE_symbol(tdbb *threadData, Database*, const TEXT*, USHORT, enum sym_type, dsql_req*);
dsql_str*		MAKE_tagged_string(tdbb *threadData, const char* str, size_t length, const char* charset);
dsql_nod*		MAKE_trigger_type(tdbb *threadData, dsql_nod*, dsql_nod*);
dsql_nod*		MAKE_variable(tdbb *threadData, dsql_fld*, const TEXT*, USHORT, USHORT, USHORT, USHORT);


#endif // DSQL_MAKE_PROTO_H

