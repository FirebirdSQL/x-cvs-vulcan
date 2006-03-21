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

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "GenericMove.h"
#include "gen/iberror.h"

GenericMove::GenericMove(FPTR_ERROR err)
{
	errorCallback = err;
	stringSpace = stringBuffer;
}

GenericMove::~GenericMove(void)
{
}


void GenericMove::arithmeticException(void)
{
	errorCallback (isc_arith_except, 0);
}

void GenericMove::wishList(void)
{
	errorCallback (isc_wish_list, isc_arg_gds, isc_blobnotsup,
			isc_arg_string, "move", 0);
}
void GenericMove::conversionError(const dsc* desc)
{
	const char* p;
	TEXT s[40];

	if (desc->dsc_dtype == dtype_blob)
		p = "BLOB";
	else if (desc->dsc_dtype == dtype_array)
		p = "ARRAY";
	else 
		{
        const char* string;
		const USHORT length = makeString (desc, ttype_ascii, &string, (vary*) s, sizeof (s));
		p = errorString(string);
		}

	errorCallback (isc_convert_error, isc_arg_string, p, 0);
}

void GenericMove::dateRangeExceeded(void)
{
	errorCallback (isc_date_range_exceeded, 0);
}

void GenericMove::notImplemented(void)
{
	//fb_assert(FALSE);
}

const char* GenericMove::errorString(const char* inputString)
{
	int length = (int) strlen (inputString);
	
	if (length > sizeof (stringBuffer))
		length = sizeof (stringBuffer - 1);
		
	if (stringSpace + length + 1 > stringBuffer + sizeof (stringBuffer))
		stringSpace = stringBuffer;
	
	const char *newString = stringSpace;
	memcpy (stringSpace, inputString, length);
	stringSpace [length] = 0;
	stringSpace += length + 1;
	
	return newString;	
}
