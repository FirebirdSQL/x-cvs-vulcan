// ActionRepeat.h: interface for the ActionRepeat class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONREPEAT_H__E11366AF_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONREPEAT_H__E11366AF_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class ActionRepeat : public Action  
{
public:
	virtual Action* eval(Stat *stat);
	virtual Action* compile(Context *context);
	ActionRepeat();
	virtual ~ActionRepeat();

	Action	*end;
};

#endif // !defined(AFX_ACTIONREPEAT_H__E11366AF_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
