// ActionTimer.h: interface for the ActionTimer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONTIMER_H__E11366BA_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONTIMER_H__E11366BA_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"
#include "Stat.h"

class ActionTimer : public Action  
{
public:
	virtual Action* compile (Context *context);
	virtual Action* eval(Stat *stat);
	ActionTimer();
	virtual ~ActionTimer();

	Action	*end;
	long	counts [stat_max];
};

#endif // !defined(AFX_ACTIONTIMER_H__E11366BA_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
