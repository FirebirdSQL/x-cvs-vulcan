// IscResultSet.h: interface for the IscResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCRESULTSET_H__C19738BA_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCRESULTSET_H__C19738BA_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "LinkedList.h"
#include "Values.h"
#include "DateTime.h"	// Added by ClassView
#include "TimeStamp.h"	// Added by ClassView

class IscStatement;
class IscResultSetMetaData;
class IscDatabaseMetaData;
class Sqlda;

class IscResultSet : public ResultSet  
{
public:
	void allocConversions();
	IscResultSet(IscStatement *iscStatement);
	virtual ~IscResultSet();
	virtual const char* getString (const char *columnName);
	virtual long		getInt (const char *columnName);
	virtual int			findColumn (const char *columName);
	virtual long		getInt (int id);
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
	virtual Time getTime (const char * columnName);
	virtual Time getTime (int index);
	virtual DateTime getDate (const char * columnName);
	virtual DateTime getDate (int index);
	virtual float getFloat (const char * columnName);
	virtual float getFloat (int id);
	virtual char getByte (const char *columnName);
	virtual char getByte (int id);
	virtual Blob* getBlob(const char * columnName);
	virtual double getDouble(const char * columnName);
	virtual double getDouble (int index);
	virtual QUAD getLong(const char * columnName);
	virtual QUAD getLong (int index);
	virtual short getShort (const char * columnName);
	virtual short getShort (int index);
	void setValue (int index, long value);
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
	Sqlda			*sqlda;
	IscStatement	*statement;
	IscResultSetMetaData *metaData;
};

#endif // !defined(AFX_ISCRESULTSET_H__C19738BA_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
