// ActionWait.h: interface for the ActionWait class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONWAIT_H__E11366B7_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONWAIT_H__E11366B7_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class ActionThread;

class ActionWait : public Action  
{
public:
	virtual Action* compile (Context *context);
	virtual Action* eval(Stat *stat);
	ActionWait();
	virtual ~ActionWait();

	int				count;
	ActionThread	**threads;
};

#endif // !defined(AFX_ACTIONWAIT_H__E11366B7_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
