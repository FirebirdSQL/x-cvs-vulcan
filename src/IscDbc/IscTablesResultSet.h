// IscTablesResultSet.h: interface for the IscTablesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCTABLESRESULTSET_H__6C3E2AB7_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCTABLESRESULTSET_H__6C3E2AB7_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscTablesResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool next();
	void getTables(const char * catalog, const char * schemaPattern, const char * tableNamePattern, int typeCount, const char **types);
	IscTablesResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscTablesResultSet();

};

#endif // !defined(AFX_ISCTABLESRESULTSET_H__6C3E2AB7_229F_11D4_98DF_0000C01D2301__INCLUDED_)
