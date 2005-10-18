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
 *
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *
 *	2003-03-24	InternalResultSet.cpp
 *				Contributed by Andrew Gough
 *				In InternalResultSet::reset() delete the 'conversions' 
 *				array itself as well as the array elements.
 *
 */

#include <stdlib.h>
#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "InternalPreparedStatement.h"
#include "SQLError.h"
#include "InternalResultSet.h"
#include "InternalConnection.h"
#include "BinaryBlob.h"
#include "Value.h"
#include "InternalStatementMetaData.h"
#include "../dsql/dsql.h"

#define THROW_ISC_EXCEPTION(connection, statusVector) throw SQLEXCEPTION (SQLError [1], connection->getInternalStatusText (statusVector))

InternalPreparedStatement::InternalPreparedStatement(InternalConnection *connection) : InternalStatement (connection)
{
	statementMetaData = NULL;
}

InternalPreparedStatement::~InternalPreparedStatement()
{
	if (statementMetaData)
		delete statementMetaData;
}

ResultSet* InternalPreparedStatement::executeQuery()
{
	execute();
	getMoreResults();

	return getResultSet();
}

Value* InternalPreparedStatement::getParameter(int index)
{
	if (index < 0 || index >= parameters.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid parameter index %d", index);

	return parameters.values + index;
}

void InternalPreparedStatement::setInt(int index, int value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setNull(int index, int type)
{
	getParameter (index - 1)->clear();
}

void InternalPreparedStatement::setDate(int index, DateTime value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setDouble(int index, double value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setString(int index, const char * string)
{
	getParameter (index - 1)->setString (string, true);
}

bool InternalPreparedStatement::execute()
{
	/***
	int numberParameters = inputSqlda.getColumnCount();

	for (int n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, connection);
	***/
	
	return InternalStatement::execute();
}

int InternalPreparedStatement::executeUpdate()
{
	return InternalStatement::execute();
}

void InternalPreparedStatement::setBytes(int index, int length, const void* bytes)
{
	BinaryBlob *blob = new BinaryBlob();
	getParameter (index - 1)->setValue (blob);
	blob->putSegment (length, (char*) bytes, true);
	blob->release();
}


bool InternalPreparedStatement::execute (const char *sqlString)
{
	return InternalStatement::execute (sqlString);
}

ResultSet*	 InternalPreparedStatement::executeQuery (const char *sqlString)
{
	return InternalStatement::executeQuery (sqlString);
}

int	InternalPreparedStatement::getUpdateCount()
{
	return InternalStatement::getUpdateCount ();
}

bool InternalPreparedStatement::getMoreResults()
{
	return InternalStatement::getMoreResults();
}

void InternalPreparedStatement::setCursorName (const char *name)
{
	InternalStatement::setCursorName (name);
}

ResultSet* InternalPreparedStatement::getResultSet()
{
	return InternalStatement::getResultSet ();
}

ResultList* InternalPreparedStatement::search (const char *searchString)
{
	return InternalStatement::search (searchString);
}

int	InternalPreparedStatement::executeUpdate (const char *sqlString)
{
	return InternalStatement::executeUpdate (sqlString);
}

void InternalPreparedStatement::close()
{
	InternalStatement::close ();
}

int InternalPreparedStatement::release()
{
	return InternalStatement::release ();
}

void InternalPreparedStatement::addRef()
{
	InternalStatement::addRef ();
}


void InternalPreparedStatement::prepare(const char * sqlString)
{
	prepareStatement (sqlString);
	getInputParameters();
}

void InternalPreparedStatement::getInputParameters()
{
	/***
	ISC_STATUS statusVector [20];

	isc_dsql_describe_bind (statusVector, &statementHandle, 1, inputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	if (inputSqlda.checkOverflow())
		{
		isc_dsql_describe_bind (statusVector, &statementHandle, 1, inputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}
	***/

	parameters.alloc (numberParameters);
}

StatementMetaData* InternalPreparedStatement::getStatementMetaData()
{
	if (statementMetaData)
		return statementMetaData;

	statementMetaData = new InternalStatementMetaData (this);

	return statementMetaData;
}

void InternalPreparedStatement::setByte(int index, char value)
{
	getParameter (index - 1)->setValue ((short) value);
}

void InternalPreparedStatement::setLong(int index, INT64 value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setFloat(int index, float value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setTime(int index, SqlTime value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setTimestamp(int index, TimeStamp value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setShort(int index, short value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setBlob(int index, Blob * value)
{
	getParameter (index - 1)->setValue (value);
}

void InternalPreparedStatement::setClob(int index, Clob * value)
{
	getParameter (index - 1)->setValue (value);
}

int InternalPreparedStatement::objectVersion()
{
	return PREPAREDSTATEMENT_VERSION;
}

void InternalPreparedStatement::mapParameters(dsql_msg* message)
{
	par *parameter = message->msg_par_ordered;
	
	for (int n = 0; parameter && n < numberParameters; ++n)
		{
		Value *value = parameters.values + n;
		dsc desc = parameter->par_desc;
		UCHAR *p = desc.dsc_address = sendBuffer + (long) desc.dsc_address;
		
		if (!(parameter = parameter->par_ordered))
			break;
	
		dsc flag = parameter->par_desc;
		flag.dsc_address = sendBuffer + (long) flag.dsc_address;
		*((short*) flag.dsc_address) = 0;
		parameter = parameter->par_ordered;
		value->getValue(&desc);
		}

}

void InternalPreparedStatement::setDescriptor(int index , dsc* value)
{
	getParameter (index - 1)->setValue (value, this);
}
