// ActionExecute.h: interface for the ActionExecute class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONEXECUTE_H__E11366B8_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONEXECUTE_H__E11366B8_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class ActionThread;

class ActionExecute : public Action  
{
public:
	virtual void fini();
	virtual Action* eval(Stat *stat);
	virtual Action* compile (Context *context);
	ActionExecute(CString name, Action *execAction);
	virtual ~ActionExecute();

	CString			threadName;
	ActionThread	*thread;
	Action			*action;
};

#endif // !defined(AFX_ACTIONEXECUTE_H__E11366B8_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
