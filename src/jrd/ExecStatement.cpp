/*
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
 *  Copyright (c) 2005 James A. Starkey
 *
 *  All Rights Reserved.
 */

//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "common.h"
#include "ExecStatement.h"
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/dsc.h"
#include "../jrd/evl_proto.h"
#include "Connection.h"
#include "InternalConnection.h"

ExecStatement::ExecStatement(Request *req)
{
	request = req;
	statement = NULL;
	resultSet = NULL;
}

ExecStatement::~ExecStatement(void)
{
	reset();
}

void ExecStatement::open(jrd_nod *sqlNode, bool singleton)
{
	reset();
	dsc *desc = EVL_expr(request->req_tdbb, sqlNode);
	const char *p = (const char*) desc->dsc_address;
	JString string;
	
	switch (desc->dsc_dtype)
		{
		case dtype_text:
			string = JString(p, desc->dsc_length);
			break;
		
		case dtype_cstring:
			string = p;
			break;
		
		case dtype_varying:
			string = JString(p + sizeof (short), ((short*) p) [0]);
			break;
		}
	
	if (statement && sqlString == string)
		return;
	
	reset();
	Connection *connection = request->req_attachment->getUserConnection(request->req_transaction);
	statement = connection->prepareStatement(string);
	sqlString = string;
}

void ExecStatement::close(void)
{
}

bool ExecStatement::fetch(jrd_nod* valueList)
{
	return false;
}

void ExecStatement::reset(void)
{
	if (resultSet)
		resultSet->close();
		
	if (statement)
		statement->close();
}
