// Action.h: interface for the Action class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTION_H__E11366A3_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTION_H__E11366A3_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

enum ActionType {
    actConnect,
	actPrepare,
	actExecuteQuery,
	actExecuteUpdate,
	actNext,
	actExecuteScript,
	actGenHtml,
	actRepeat,
	actWhile,
	actEnd,
	actScript,
	actThread,
	actThreadExecute,
	actWait,
	actTimer,
	actPause,
	actStatement,
	};

class Arg;
class Context;
class Stat;


class Action  
{
protected:
	virtual ~Action();

public:
	bool getBoolArg (const char*argName, bool defaultValue);
	void release();
	void addRef();
	int getIntArg (const char *argName, int defaultValue);
	virtual void fini();
	virtual Action* eval(Stat *stat) = 0;
	virtual Action* compile(Context *context);
	virtual const char * getStringArg (const char *argName, const char *defaultValue);
	void addArgument (Arg *arg);
	virtual void addArgument (CString argName, CString argValue);
	void setName (CString string);
	Action(ActionType actionType);

    ActionType	type;
	Action		*next;
	Action		*que;
	CString		thread;
    CString		name;
	Arg			*args;
	int			level;
	int			useCount;
};

#endif // !defined(AFX_ACTION_H__E11366A3_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
