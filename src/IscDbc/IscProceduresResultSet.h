// IscProceduresResultSet.h: interface for the IscProceduresResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCPROCEDURESRESULTSET_H__32C6E497_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCPROCEDURESRESULTSET_H__32C6E497_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscProceduresResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool next();
	void getProcedures(const char * catalog, const char * schemaPattern, const char * procedureNamePattern);
	IscProceduresResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscProceduresResultSet();

};

#endif // !defined(AFX_ISCPROCEDURESRESULTSET_H__32C6E497_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
