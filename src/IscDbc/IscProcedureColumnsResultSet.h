// IscProcedureColumnsResultSet.h: interface for the IscProcedureColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCPROCEDURECOLUMNSRESULTSET_H__32C6E498_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCPROCEDURECOLUMNSRESULTSET_H__32C6E498_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscProcedureColumnsResultSet : public IscMetaDataResultSet  
{
public:
	typedef IscMetaDataResultSet Parent;

	virtual int getPrecision (int index);
	virtual int getColumnType (int index);
	virtual int getColumnDisplaySize(int index);
	virtual bool next();
	void getProcedureColumns (const char *catalog, 
							  const char *schemaPattern, 
							  const char *procedureNamePattern, 
							  const char *columnNamePattern);
	IscProcedureColumnsResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscProcedureColumnsResultSet();

};

#endif // !defined(AFX_ISCPROCEDURECOLUMNSRESULTSET_H__32C6E498_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
