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

#ifndef _INTERNALRESULTSET_H_
#define _INTERNALRESULTSET_H_

#include "Connection.h"
#include "Values.h"
#include "LinkedList.h"
//#include "DateTime.h"	// Added by ClassView
//#include "TimeStamp.h"	// Added by ClassView

class InternalStatement;
class InternalResultSetMetaData;
class InternalDatabaseMetaData;
class dsql_msg;
struct dsc;

class InternalResultSet : public ResultSet  
{
public:
	void allocConversions();
	InternalResultSet(InternalStatement *iscStatement);
	virtual ~InternalResultSet();
	virtual const char* getString (const char *columnName);
	virtual int			getInt (const char *columnName);
	virtual int			findColumn (const char *columName);
	virtual int			getInt (int id);
	virtual void		freeHTML(const char *html);
	virtual const char* genHTML(const char *series, const char *type, Properties *context);
	virtual Blob*		getBlob (int index);
	virtual void		close();
	virtual const char* getString (int id);
	virtual bool		next();
	virtual ResultSetMetaData* getMetaData();
	virtual int			release();
	virtual void		addRef();
	virtual bool		wasNull();

	virtual bool		isNullable (int index);
	virtual int			getScale (int index);
	virtual int			getPrecision (int index);
	virtual const char* getTableName (int index);
	virtual const char* getColumnName (int index);
	virtual int			getColumnDisplaySize (int index);
	virtual int			getColumnType (int index);

	virtual Value*		getValue (int index);
	virtual Value*		getValue (const char *columnName);

	void		deleteBlobs();
	void		reset();

public:
	virtual Clob* getClob (const char* columnName);
	virtual Clob* getClob (int index);
	virtual int objectVersion();
	virtual TimeStamp getTimestamp (const char * columnName);
	virtual TimeStamp getTimestamp (int index);
	virtual SqlTime getTime (const char * columnName);
	virtual SqlTime getTime (int index);
	virtual DateTime getDate (const char * columnName);
	virtual DateTime getDate (int index);
	virtual float getFloat (const char * columnName);
	virtual float getFloat (int id);
	virtual char getByte (const char *columnName);
	virtual char getByte (int id);
	virtual Blob* getBlob(const char * columnName);
	virtual double getDouble(const char * columnName);
	virtual double getDouble (int index);
	virtual INT64 getLong(const char * columnName);
	virtual INT64 getLong (int index);
	virtual short getShort (const char * columnName);
	virtual short getShort (int index);
	void setValue (int index, int value);
	void setValue (int index, const char *value);

	int				numberColumns;
	void			*handle;
	int				useCount;
	Values			values;
	char			**conversions;
	char			**columnNames;
	bool			valueWasNull;
	LinkedList		blobs;
	LinkedList		clobs;
	UCHAR			*buffer;
	dsql_msg		*message;
	dsc				*descriptors;
	InternalStatement	*statement;
	InternalResultSetMetaData *metaData;
	//virtual dsc getDescriptor(int id);
	//virtual dsc getDescriptor(const char* columnName);
};

#endif
