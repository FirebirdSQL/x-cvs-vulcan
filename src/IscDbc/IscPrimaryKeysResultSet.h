// IscPrimaryKeysResultSet.h: interface for the IscPrimaryKeysResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCPRIMARYKEYSRESULTSET_H__32C6E493_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCPRIMARYKEYSRESULTSET_H__32C6E493_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscPrimaryKeysResultSet : public IscMetaDataResultSet  
{
public:
	virtual bool next();
	void getPrimaryKeys (const char * catalog, const char * schemaPattern, const char * tableNamePattern);
	IscPrimaryKeysResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscPrimaryKeysResultSet();

};

#endif // !defined(AFX_ISCPRIMARYKEYSRESULTSET_H__32C6E493_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
