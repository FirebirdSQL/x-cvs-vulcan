// ActionScript.h: interface for the ActionScript class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONSCRIPT_H__E11366B3_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONSCRIPT_H__E11366B3_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"

class Script;


class ActionScript : public Action  
{
public:
	virtual void fini();
	virtual Action* eval(Stat *stat);
	virtual Action* compile(Context *context);
	ActionScript(Script *scrpt);
	virtual ~ActionScript();

	Script	*script;
	Action	*actions;
};

#endif // !defined(AFX_ACTIONSCRIPT_H__E11366B3_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
