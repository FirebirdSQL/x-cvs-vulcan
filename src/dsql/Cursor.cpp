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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */
 
#include <string.h>
#include "firebird.h"
#include "common.h"
#include "Cursor.h"
#include "ibase.h"
#include "OSRIException.h"

Cursor::Cursor(DStatement *state, Transaction *trans)
{
	statement = state;
	transaction = trans;
}

Cursor::~Cursor(void)
{
}

JString Cursor::getCursorName(const char* cursorName)
{
    const TEXT *p = cursorName;
	TEXT cursor[132];
	TEXT *q = cursor;
	
	if (strlen(cursorName) >= sizeof(cursor))
		throw OSRIException(isc_sqlerr, isc_arg_number, -502,
							isc_arg_gds, isc_dsql_decl_err, 0);
				  
    
	if (*p == '\"')
		{
		// Quoted cursor names eh? Strip'em.
		// Note that "" will be replaced with ".
		//
		
		for (; *p; ++p)
			{
			if (*p == '"')
				++p;
			*q++ = *p;
			}
		}
	else	// not quoted name
		for (; *p && *p != ' '; ++p)
			*q++ = UPPER7(*p);

	*q = 0;	
	
	return cursor;
}
