/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#include "fbdev.h"
#include "common.h"
#include "BlrParse.h"
#include "dsc.h"
#include "align.h"
#include "blr.h"

BlrParse::BlrParse(const UCHAR *blrPtr)
{
	blr = ptr = blrPtr;
	version = -1;
}

BlrParse::~BlrParse()
{
}

void BlrParse::getBlrDescriptor(dsc* desc)
{
	int blrType = *ptr++;
	desc->dsc_dtype = gds_cvt_blr_dtype [blrType];
	desc->dsc_scale = 0;
	desc->dsc_sub_type = 0;
	
	switch (blrType)
		{
		case blr_text:
		case blr_cstring:
			desc->dsc_length = getWord ();
			break;
			
		case blr_varying:
			desc->dsc_length = getWord () + 2;
			break;
			
		case blr_short:
		case blr_long:
		case blr_quad:
		case blr_int64:
			desc->dsc_scale = (SCHAR) *ptr++;
		case blr_timestamp:
		case blr_sql_time:
		case blr_sql_date:
		case blr_d_float:
		case blr_double:
		case blr_float:
			desc->dsc_length = type_lengths [desc->dsc_dtype];	
			break;
		}
}

int BlrParse::getVersion(void)
{
	return version = getByte();
}
