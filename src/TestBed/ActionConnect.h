// ActionConnect.h: interface for the ActionConnect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONCONNECT_H__E11366AD_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONCONNECT_H__E11366AD_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"
#include "LinkedList.h"
#include "..\ENGINE\JString.h"	// Added by ClassView

class Connection;
class ActionStatement;

class ActionConnect : public Action  
{
public:
	void addStatement (ActionStatement *statement);
	JString getCookies();
	void processCookies (const char *html);
	virtual void fini();
	virtual Action* compile (Context *context);
	virtual void close();
	virtual Action* eval(Stat *stat);
	ActionConnect();
	virtual ~ActionConnect();

	Connection		*connection;
	LinkedList		cookies;
	ActionStatement	*statements;
	bool			autoCommit;
};

#endif // !defined(AFX_ACTIONCONNECT_H__E11366AD_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
