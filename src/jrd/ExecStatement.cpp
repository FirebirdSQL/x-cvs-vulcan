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

#include "fbdev.h"
#include "common.h"
#include "ExecStatement.h"
#include "../jrd/jrd.h"
#include "../jrd/req.h"
#include "../jrd/dsc.h"
#include "Connection.h"
#include "InternalConnection.h"
#include "InternalPreparedStatement.h"
#include "InternalResultSet.h"
#include "Value.h"
#include "Attachment.h"
#include "err_proto.h"
#include "gen/iberror.h"
#include "../jrd/evl_proto.h"
#include "../jrd/mov_proto.h"

ExecStatement::ExecStatement(Request *req)
{
	request = req;
	statement = 0;
	resultSet = 0;
	connection = 0;
}

ExecStatement::~ExecStatement(void)
{
	reset();

	if (connection)
		request->req_attachment->closeConnection(connection);
}

void ExecStatement::prepare(jrd_nod *sqlNode, bool singletonFlag)
{
//	reset();
	singleton = singletonFlag;
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
	
	if (statement && (sqlString == string))
		return;
	
	reset();
	if (!connection)
		connection = request->req_attachment->getUserConnection(request->req_transaction);
	statement = connection->prepareStatement(string);
	sqlString = string;
	StatementMetaData *metaData = statement->getStatementMetaData();
	numberParameters = metaData->getParameterCount();
}

void ExecStatement::execute(jrd_nod* list)
{
	if (numberParameters > list->nod_count)
		ERR_post(isc_wronumarg, 0);

	/***		
	for (int n = 0; n < numberParameters; ++n)
		{
		dsc *desc = EVL_expr(request->req_tdbb, list->nod_arg[n]);
		((InternalPreparedStatement*) statement)->setDescriptor(n + 1, desc);
		}
	***/
	
	first = true;
	statement->execute();
}

bool ExecStatement::fetch(jrd_nod* valueList)
{
	if (!resultSet)
		{
		resultSet = statement->getResultSet();
		ResultSetMetaData *metaData = resultSet->getMetaData();
		numberColumns = metaData->getColumnCount();
		}
	
	if (!resultSet->next())
		{
		if (singleton && first)
			ERR_post(isc_sing_select_err, 0);
			
		return false;
		}
	
	first = false;
	
	for (int n = 0; n < numberColumns; ++n)
		{
		//dsc *to = EVL_assign_to(request->req_tdbb, valueList->nod_arg[numberParameters + n]);
		dsc *to = EVL_assign_to(request->req_tdbb, valueList->nod_arg[n]);
		Value *value = ((InternalResultSet*) resultSet)->getValue(n + 1);
		
		if (value->getValue(to))
			to->dsc_flags |= DSC_null;
		else
			to->dsc_flags &= ~DSC_null;
		}

	return true;
}

void ExecStatement::close(void)
{
}

void ExecStatement::reset(void)
{
	if (resultSet)
	{
		resultSet->close();
		resultSet = 0;
	}
		
	if (statement)
	{
		statement->close();
		statement = 0;
	}
}
