// Script.h: interface for the Script class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPT_H__B96C96D8_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_SCRIPT_H__B96C96D8_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ApplicationObject.h"

class Action;
class Context;
class Stat;

class Script : public ApplicationObject  
{
public:
	virtual Script* findScript(ApplicationObject *object, CString scriptName);
	void init();
	void fini(Action *actions);
	virtual Action* compile(Context *context);
	virtual void eval(Action *actions, Stat *stat);
	virtual void getArguments (Action *action);
	virtual void update();
	bool match (const char *string);
	Action* parse();
	void clear();
	bool getToken();
	bool skipWhite();
	virtual void execute();
	virtual CEditor* createEditWindow();
	virtual CString getText();
	virtual void update(const char * text);
	virtual bool doubleClick (TreeNode *node);
	 Script();
	Script(const char *scriptName, ApplicationObject *parent);
	virtual ~Script();

	CString		body;
	//Action		*actions;
	const char	*text;
	bool		eol;
	bool		compiled;
	CString		token;
};

#endif // !defined(AFX_SCRIPT_H__B96C96D8_0584_11D4_98DB_0000C01D2301__INCLUDED_)
