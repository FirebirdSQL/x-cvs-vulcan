// ActionEnd.h: interface for the ActionEnd class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONEND_H__E11366B0_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONEND_H__E11366B0_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class ActionEnd : public Action  
{
public:
	virtual Action* eval(Stat *stat);
	ActionEnd();
	virtual ~ActionEnd();

};

#endif // !defined(AFX_ACTIONEND_H__E11366B0_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
