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
// InternalResultSet.cpp: implementation of the InternalResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "firebird.h"
#include "InternalResultSet.h"
#include "InternalResultSetMetaData.h"
#include "InternalStatement.h"
#include "InternalConnection.h"
#include "SQLError.h"
#include "Value.h"
#include "../dsql/CStatement.h"
#include "../dsql/dsql.h"
#include "jrd_proto.h"
#include "OSRIException.h"

#define NOT_YET_IMPLEMENTED	throw SQLError (FEATURE_NOT_YET_IMPLEMENTED, "not yet implemented")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


InternalResultSet::InternalResultSet(InternalStatement *iscStatement)
{
	useCount = 1;
	statement = iscStatement;
	metaData = NULL;
	conversions = NULL;
	buffer = NULL;
	message = NULL;
	descriptors = NULL;
	
	if (statement)
		{
		numberColumns = statement->numberColumns;
		values.alloc (numberColumns);
		allocConversions();
		statement->addRef();
		if (message = statement->statement->receiveMessage)
			buffer = new UCHAR [message->msg_length];
		}
}

InternalResultSet::~InternalResultSet()
{
	close();

	if (buffer)
		delete [] buffer;

	if (metaData)
		delete metaData;
	
	if (descriptors)
		delete [] descriptors;
	
	if (conversions)
		delete [] conversions;
}

ResultSetMetaData* InternalResultSet::getMetaData()
{
	if (metaData)
		return (ResultSetMetaData*) metaData;

	metaData = new InternalResultSetMetaData (this, numberColumns);

	return (ResultSetMetaData*) metaData;
}

bool InternalResultSet::next()
{
	deleteBlobs();
	reset();
	ISC_STATUS statusVector [20];
	
	if (jrd8_receive (statusVector, 
						&statement->statement->request,
						message->msg_number, 
						message->msg_length, 
						buffer, 
						statement->requestInstantiation))
		throw OSRIException(statusVector);
		
	short *eof = (short*) (buffer + (long) statement->statement->req_eof->par_desc.dsc_address);

	if (!*eof)
		return false;

	return true;
}

const char* InternalResultSet::getString(int id)
{
	if (id < 1 || id > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	if (conversions [id - 1])
		return conversions [id - 1];

	return getValue (id)->getString(conversions + id - 1);
}


const char* InternalResultSet::getString(const char * columnName)
{
	return getString (findColumn (columnName));
}

int InternalResultSet::getInt(int id)
{
	return getValue (id)->getLong();
}

int InternalResultSet::getInt(const char * columnName)
{
	return getValue (columnName)->getLong();
}

Value* InternalResultSet::getValue(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	Value *value = values.values + index - 1;
	int sequence = (index - 1) * 2;
	par *parameter = message->msg_par_ordered;
	
	for (int n = 0; n < sequence; ++n)
		parameter = parameter->par_ordered;
		
	dsc desc = parameter->par_desc;
	desc.dsc_address = buffer + (long) desc.dsc_address;
	value->setValue(&desc, statement);
	valueWasNull = value->type == Null;

	return value;
}

Value* InternalResultSet::getValue(const char * columnName)
{
	return getValue (findColumn (columnName));
}

void InternalResultSet::close()
{
	if (statement)
		{
		if (statement->connection->autoCommit)
			statement->connection->commitAuto();
		statement->deleteResultSet (this);
		statement->release();
		statement = NULL;
		}
	
	reset();
}

Blob* InternalResultSet::getBlob(int index)
{
	Blob *blob = getValue (index)->getBlob();
	blobs.append (blob);

	return blob;
}

Blob* InternalResultSet::getBlob(const char * columnName)
{
	Blob *blob = getValue (columnName)->getBlob();
	blobs.append (blob);

	return blob;
}


Clob* InternalResultSet::getClob(int index)
{
	Clob *blob = getValue (index)->getClob();
	clobs.append (blob);

	return blob;
}

Clob* InternalResultSet::getClob(const char * columnName)
{
	Clob *blob = getValue (columnName)->getClob();
	clobs.append (blob);

	return blob;
}

void InternalResultSet::deleteBlobs()
{
	FOR_OBJECTS (Blob*, blob, &blobs)
		blob->release();
	END_FOR;

	blobs.clear();

	FOR_OBJECTS (Clob*, blob, &clobs)
		blob->release();
	END_FOR;

	clobs.clear();
}

const char* InternalResultSet::genHTML(const char *series, const char *type, Properties *context)
{
	throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "function is not implemented");

	return NULL;
}

void InternalResultSet::freeHTML(const char *html)
{
	delete [] (char*) html;
}

void InternalResultSet::addRef()
{
	++useCount;
}

int InternalResultSet::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

void InternalResultSet::reset()
{
	if (conversions)
		for (int n = 0; n < numberColumns; ++n)
			if (conversions [n])
				{
				delete [] conversions [n];
				conversions [n] = NULL;
				}
}

int InternalResultSet::findColumn(const char * columnName)
{
	int index = statement->findColumn(columnName);
	
	if (index >= 0)
		return index;
			
	throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column name %s for result set", columnName);

	return -1;
}



bool InternalResultSet::wasNull()
{
	return valueWasNull;
}

void InternalResultSet::allocConversions()
{
	conversions = new char* [numberColumns];
	memset (conversions, 0, sizeof (char*) * numberColumns);
}

void InternalResultSet::setValue(int index, const char * value)
{
	values.values [index - 1].setString (value, true);
}

void InternalResultSet::setValue(int index, int value)
{
	values.values [index - 1].setValue (value, true);
}

short InternalResultSet::getShort(int id)
{
	return getValue (id)->getShort();
}

short InternalResultSet::getShort(const char * columnName)
{
	return getValue (columnName)->getShort();
}

INT64 InternalResultSet::getLong(int id)
{
	return getValue (id)->getQuad();
}

INT64 InternalResultSet::getLong(const char * columnName)
{
	return getValue (columnName)->getQuad();
}

double InternalResultSet::getDouble(int id)
{
	return getValue (id)->getDouble();
}

double InternalResultSet::getDouble(const char * columnName)
{
	return getValue (columnName)->getDouble();
}

char InternalResultSet::getByte(int id)
{
	return getValue (id)->getByte();
}

char InternalResultSet::getByte(const char * columnName)
{
	return getValue (columnName)->getByte();
}

float InternalResultSet::getFloat(int id)
{
	return (float) getValue (id)->getDouble();
}

float InternalResultSet::getFloat(const char * columnName)
{
	return (float) getValue (columnName)->getDouble();
}

int InternalResultSet::getColumnType(int index)
{
	//return sqlda->getColumnType (index);
	NOT_YET_IMPLEMENTED;
}

int InternalResultSet::getColumnDisplaySize(int index)
{
	//return sqlda->getDisplaySize (index);
	NOT_YET_IMPLEMENTED;
}

const char* InternalResultSet::getColumnName(int index)
{
	//return sqlda->getColumnName (index);
	NOT_YET_IMPLEMENTED;
}

const char* InternalResultSet::getTableName(int index)
{
	//return sqlda->getTableName (index);
	NOT_YET_IMPLEMENTED;
}

int InternalResultSet::getPrecision(int index)
{
	//return sqlda->getPrecision (index);
	NOT_YET_IMPLEMENTED;
}

int InternalResultSet::getScale(int index)
{
	//return sqlda->getScale (index);
	NOT_YET_IMPLEMENTED;
}

bool InternalResultSet::isNullable(int index)
{
	//return sqlda->isNullable (index);
	NOT_YET_IMPLEMENTED;
}

DateTime InternalResultSet::getDate(int id)
{
	return getValue (id)->getDate();
}

DateTime InternalResultSet::getDate(const char * columnName)
{
	return getValue (columnName)->getDate();
}

SqlTime InternalResultSet::getTime(int id)
{
	return getValue (id)->getTime();
}

SqlTime InternalResultSet::getTime(const char * columnName)
{
	return getValue (columnName)->getTime();
}

TimeStamp InternalResultSet::getTimestamp(int id)
{
	return getValue (id)->getTimestamp();
}

TimeStamp InternalResultSet::getTimestamp(const char * columnName)
{
	return getValue (columnName)->getTimestamp();
}

int InternalResultSet::objectVersion()
{
	return RESULTSET_VERSION;
}
