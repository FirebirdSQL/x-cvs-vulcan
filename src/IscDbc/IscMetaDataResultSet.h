// IscMetaDataResultSet.h: interface for the IscMetaDataResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCMETADATARESULTSET_H__6C3E2AB8_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCMETADATARESULTSET_H__6C3E2AB8_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscResultSet.h"
#include "JString.h"	// Added by ClassView

class IscDatabaseMetaData;
class IscResultSet;

class IscMetaDataResultSet : public IscResultSet  
{
public:
	virtual bool next();
	JString expandPattern (const char *string, const char *pattern);
	bool isWildcarded (const char *pattern);
	void trimBlanks (int id);
	virtual void prepareStatement (const char *sql);
	virtual Value* getValue (int index);
	virtual int findColumn (const char *columnName);
	IscMetaDataResultSet(IscDatabaseMetaData *meta);
	virtual ~IscMetaDataResultSet();

	virtual bool		isNullable (int index);
	virtual int			getScale (int index);
	virtual int			getPrecision (int index);
	virtual const char* getTableName (int index);
	virtual const char* getColumnName (int index);
	virtual int			getColumnDisplaySize (int index);
	virtual int			getColumnType (int index);


	IscDatabaseMetaData	*metaData;
	IscResultSet		*resultSet;
	PreparedStatement	*statement;
};

#endif // !defined(AFX_ISCMETADATARESULTSET_H__6C3E2AB8_229F_11D4_98DF_0000C01D2301__INCLUDED_)
