// ActionScript.cpp: implementation of the ActionScript class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionScript.h"
#include "Script.h"
#include "Context.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionScript::ActionScript(Script *scrpt) : Action (actScript)
{
	script = scrpt;
	actions = NULL;
}

ActionScript::~ActionScript()
{
	if (actions)
		actions->release();
}

Action* ActionScript::compile(Context *context)
{
	Action *mark = context->mark();
	actions = script->compile (context);
	context->revert (mark);

	return next;
}	

Action* ActionScript::eval (Stat *stat)
{
	script->eval (actions, stat);

	return next;
}

void ActionScript::fini()
{
	script->fini (actions);
}
