// IscResultSetMetaData.h: interface for the IscResultSetMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCRESULTSETMETADATA_H__C19738BB_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCRESULTSETMETADATA_H__C19738BB_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"

/***
struct MetaData {
    char	*columnName;
	char	*tableName;
	int		type;
	int		displaySize;
	int		scale;
	};
***/

class IscResultSet;
class Value;

class IscResultSetMetaData : public ResultSetMetaData  
{
public:
	virtual bool isDefinitelyWritable (int index);
	virtual bool isReadOnly (int index);
	virtual bool isWritable (int index);
	virtual bool isSigned (int index);
	virtual const char* getColumnLabel (int index);
	virtual int objectVersion();
	virtual bool isNullable (int index);
	virtual int getScale (int index);
	virtual int getPrecision (int index);
	IscResultSetMetaData(IscResultSet *results, int numberColumns);
	virtual ~IscResultSetMetaData();
	virtual const char* getTableName (int index);
	virtual const char* getColumnName (int index);
	virtual int getColumnDisplaySize (int index);
	virtual int getColumnType (int index);
	virtual int getColumnCount();

	//MetaData*	checkIndex(int index);

	IscResultSet	*resultSet;
	int				numberColumns;
	//MetaData		*metaData;
	char			*query;
};

#endif // !defined(AFX_ISCRESULTSETMETADATA_H__C19738BB_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
