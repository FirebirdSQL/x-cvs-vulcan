// Context.h: interface for the Context class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONTEXT_H__E11366B5_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_CONTEXT_H__E11366B5_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Action;
enum ActionType;

class Context  
{
public:
	void revert (Action *mark);
	Action* mark();
	Action* findAction (ActionType type, const char *name);
	Action* findAction (ActionType type);
	Action* pop();
	void push (Action *action);
	Context();
	virtual ~Context();

	Action	*actions;
	int		level;
};

#endif // !defined(AFX_CONTEXT_H__E11366B5_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
