// TestEnv.h: interface for the TestEnv class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TESTENV_H__B96C96D6_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_TESTENV_H__B96C96D6_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ApplicationObject.h"

class Scripts;
class CTestBedDoc;

class TestEnv : public ApplicationObject  
{
public:
	virtual void save (ApplicationObject *object);
	void setDocument (CTestBedDoc *doc);
	virtual void markChanged();
	 TestEnv (CTestBedDoc *doc);
	Scripts* scripts;
	virtual void populate();
	TestEnv();
	virtual ~TestEnv();

	CTestBedDoc	*document;

};

#endif // !defined(AFX_TESTENV_H__B96C96D6_0584_11D4_98DB_0000C01D2301__INCLUDED_)
