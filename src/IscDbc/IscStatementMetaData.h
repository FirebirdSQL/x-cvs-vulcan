// IscStatementMetaData.h: interface for the IscStatementMetaData class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCSTATEMENTMETADATA_H__32C6E496_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCSTATEMENTMETADATA_H__32C6E496_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"

class IscPreparedStatement;


class IscStatementMetaData : public StatementMetaData  
{
public:
	virtual int objectVersion();
	virtual bool isNullable (int index);
	virtual int getScale (int index);
	virtual int getPrecision (int index);
	virtual int getParameterType (int index);
	virtual int getParameterCount();
	IscStatementMetaData(IscPreparedStatement *preparedStatement);
	virtual ~IscStatementMetaData();

	IscPreparedStatement	*statement;
};

#endif // !defined(AFX_ISCSTATEMENTMETADATA_H__32C6E496_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
