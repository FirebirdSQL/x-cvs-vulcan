// IscColumnsMetaData.h: interface for the IscColumnsMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCCOLUMNSMETADATA_H__6C3E2ABA_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCCOLUMNSMETADATA_H__6C3E2ABA_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscColumnsResultSet : public IscMetaDataResultSet  
{
public:
	virtual int getPrecision (int index);
	virtual int getColumnDisplaySize(int index);
	typedef IscMetaDataResultSet Parent;

	virtual int getColumnType (int index);
	virtual bool next();
	void getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern);
	IscColumnsResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscColumnsResultSet();

};

#endif // !defined(AFX_ISCCOLUMNSMETADATA_H__6C3E2ABA_229F_11D4_98DF_0000C01D2301__INCLUDED_)
