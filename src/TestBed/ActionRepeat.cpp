// ActionRepeat.cpp: implementation of the ActionRepeat class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionRepeat.h"
#include "ScriptError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionRepeat::ActionRepeat() : Action (actRepeat)
{

}

ActionRepeat::~ActionRepeat()
{

}


Action* ActionRepeat::compile (Context *context)
{
	for (Action *action = next; action;)
		{
		if (action->type == actEnd)
			{
			end = action;
			return end->next;
			}
		action = action->compile (context);
		}

	throw ScriptError ("unterminated 'repeat' verb");

	return next;
}

Action* ActionRepeat::eval(Stat *stat)
{
	int count = atoi (getStringArg ("count", "1"));

	for (int n = 0; n < count; ++n)
		for (Action *action = next; action && action != end;)
			action = action->eval (stat);

	return end->next;
}
