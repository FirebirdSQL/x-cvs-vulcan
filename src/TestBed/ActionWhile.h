// ActionWhile.h: interface for the ActionWhile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONWHILE_H__E11366B1_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONWHILE_H__E11366B1_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class ActionWhile : public Action  
{
public:
	virtual Action* compile(Context *context);
	virtual Action* eval(Stat *stat);
	ActionWhile();
	virtual ~ActionWhile();

	Action	*end;
};

#endif // !defined(AFX_ACTIONWHILE_H__E11366B1_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
