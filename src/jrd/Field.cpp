/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Field
 *	DESCRIPTION:	Relation field class implementation
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

// Field.cpp: implementation of the Field class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "jrd.h"
#include "Field.h"
#include "../dsql/dsql_rel.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Field::Field()
{
	init();
}


Field::Field(JString name)
{
	init();
	setName(name);
}

Field::~Field()
{

}

void Field::init(void)
{
	fld_validation = NULL;
	fld_not_null = NULL;
	fld_missing_value = NULL;
	fld_computation = NULL;
	fld_source = NULL;
	fld_security_name = NULL;
	fld_default_value = NULL;
	fld_array = NULL;
	fld_dimensions = 0;
	fld_dtype = 0;
	fld_scale = 0;
	fld_length = 0;
	fld_sub_type = 0;
	fld_precision = 0;
	fld_position = 0;
	dsqlField = NULL;
}

void Field::setName(JString name)
{
	fld_name = name;
	fld_length = fld_name.length();
}

void Field::setType(DSC* desc, int dimensions)
{
	fld_dtype = desc->dsc_dtype;
	fld_scale = desc->dsc_scale;
	fld_precision = desc->dsc_length;
	fld_sub_type = desc->dsc_sub_type;
	fld_dimensions = dimensions;
	
	if (dsqlField)
		dsqlField->setType(desc, dimensions);
}
