// Attachment.h: interface for the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ATTACHMENT_H__F3F1D3A9_4083_11D4_98E8_0000C01D2301__INCLUDED_)
#define AFX_ATTACHMENT_H__F3F1D3A9_4083_11D4_98E8_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Mutex.h"

class Properties;

class Attachment  
{
public:
	int release();
	void addRef();
	void openDatabase(const char * dbName, Properties * properties);
	Attachment();
	virtual ~Attachment();

	isc_db_handle	databaseHandle;
	void			*transactionHandle;
	JString			databaseName;
	int				dialect;
	bool			quotedIdentifiers;
	JString			userName;
	JString			databaseVersion;
	int				transactionIsolation;
	int				useCount;
	Mutex			mutex;
};

#endif // !defined(AFX_ATTACHMENT_H__F3F1D3A9_4083_11D4_98E8_0000C01D2301__INCLUDED_)
