// ActionNext.h: interface for the ActionNext class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONNEXT_H__E11366B2_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONNEXT_H__E11366B2_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class ActionNext : public Action  
{
public:
	virtual Action* eval(Stat *stat);
	ActionNext();
	virtual ~ActionNext();

};

#endif // !defined(AFX_ACTIONNEXT_H__E11366B2_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
