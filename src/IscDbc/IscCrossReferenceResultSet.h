// IscCrossReferenceResultSet.h: interface for the IscCrossReferenceResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCCROSSREFERENCERESULTSET_H__32C6E495_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCCROSSREFERENCERESULTSET_H__32C6E495_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "IscMetaDataResultSet.h"

class IscCrossReferenceResultSet : public IscMetaDataResultSet  
{
public:
	bool stringEqual (const char *p1, const char *p2);
	int getRule (const char *rule);
	virtual bool next();
	void getCrossReference(const char* primaryCatalog, const char* primarySchema, const char* primaryTable,const char* foreignCatalog, const char* foreignSchema, const char* foreignTable);
	IscCrossReferenceResultSet(IscDatabaseMetaData *metaData);
	virtual ~IscCrossReferenceResultSet();

};

#endif // !defined(AFX_ISCCROSSREFERENCERESULTSET_H__32C6E495_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
