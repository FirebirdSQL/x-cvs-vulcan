// ActionPause.h: interface for the ActionPause class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONPAUSE_H__BFE37511_B681_11D4_98FA_0000C01D2301__INCLUDED_)
#define AFX_ACTIONPAUSE_H__BFE37511_B681_11D4_98FA_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Action.h"

class ActionPause : public Action  
{
public:
	virtual Action* eval(Stat *stat);
	virtual Action* compile(Context * context);
	ActionPause();
	virtual ~ActionPause();

	int		min;
	int		max;
};

#endif // !defined(AFX_ACTIONPAUSE_H__BFE37511_B681_11D4_98FA_0000C01D2301__INCLUDED_)
