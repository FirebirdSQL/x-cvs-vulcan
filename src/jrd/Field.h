/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Field.h
 *	DESCRIPTION:	Relation field class
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
 * December 27, 2003	Created by James A. Starkey
 *
 */
// Field.h: interface for the Field class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FIELD_H__B58D0A23_85E4_44D2_B3DE_0074D65B238C__INCLUDED_)
#define AFX_FIELD_H__B58D0A23_85E4_44D2_B3DE_0074D65B238C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

class jrd_nod;
class arr;
class dsql_fld;

class Field// : public blk //: public pool_alloc_rpt<SCHAR, type_fld>
{
public:
	Field();
	virtual ~Field();

    public:
	jrd_nod*	fld_validation;		/* validation clause, if any */
	jrd_nod*	fld_not_null;		/* if field cannot be NULL */
	jrd_nod*	fld_missing_value;	/* missing value, if any */
	jrd_nod*	fld_computation;	/* computation for virtual field */
	jrd_nod*	fld_source;			/* source for view fields */
	jrd_nod*	fld_default_value;	/* default value, if any */
	JString		fld_security_name;	/* pointer to security class name for field */
	arr*		fld_array;			/* array description, if array */
	JString		fld_name;			/* Field name */
	Field		*fld_junk;
	int			fld_dimensions;		/* used by DSQL */
	int			fld_dtype;			/* used by DSQL */
	int			fld_scale;			/* used by DSQL */
	int			fld_sub_type;
	int			fld_precision;
	int			fld_id;
	int			fld_position;
	UCHAR		fld_length;			/* Field name length */
	dsql_fld	*dsqlField;
	
	void init(void);
	Field(JString name);
	void setName(JString name);
	void setType(DSC* desc, int dimensions);
};

#endif // !defined(AFX_FIELD_H__B58D0A23_85E4_44D2_B3DE_0074D65B238C__INCLUDED_)
