// ActionExecute.cpp: implementation of the ActionExecute class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionExecute.h"
#include "ActionThread.h"
#include "Context.h"
#include "ScriptError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionExecute::ActionExecute(CString name, Action *execAction) : Action (actThreadExecute)
{
	threadName = name;
	action = execAction;
}

ActionExecute::~ActionExecute()
{
	if (action)
		action->release();
}

Action* ActionExecute::compile(Context * context)
{
	thread = (ActionThread*) context->findAction (actThread, threadName);

	if (!thread)
		throw ScriptError ("can't find thread \"%s\"", (const char*) threadName);

	Action *mark = context->mark();
	action->compile (context);
	context->revert (mark);

	return next;
}

Action* ActionExecute::eval(Stat *stat)
{
	thread->queAction (action, stat);

	return next;
}

void ActionExecute::fini()
{
	action->fini();
}
