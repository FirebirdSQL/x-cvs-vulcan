// IscIndexInfoResultSet.h: interface for the IscIndexInfoResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCINDEXINFORESULTSET_H__32C6E492_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCINDEXINFORESULTSET_H__32C6E492_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscIndexInfoResultSet : public IscMetaDataResultSet  
{
public:
	void getIndexInfo(const char * catalog, const char * schemaPattern, const char * tableNamePattern, bool unique, bool approximate);
	virtual bool next();
	IscIndexInfoResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscIndexInfoResultSet();

};

#endif // !defined(AFX_ISCINDEXINFORESULTSET_H__32C6E492_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
