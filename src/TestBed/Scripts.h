// Scripts.h: interface for the Scripts class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPTS_H__B96C96D7_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_SCRIPTS_H__B96C96D7_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ApplicationObject.h"

class Scripts : public ApplicationObject  
{
public:
	 Scripts();
	Scripts(ApplicationObject *parent);
	virtual ~Scripts();

};

#endif // !defined(AFX_SCRIPTS_H__B96C96D7_0584_11D4_98DB_0000C01D2301__INCLUDED_)
