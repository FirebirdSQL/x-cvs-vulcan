// ActionTimer.cpp: implementation of the ActionTimer class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "stdafx.h"
#include "TestBed.h"
#include "ActionTimer.h"
#include "ScriptError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionTimer::ActionTimer() : Action (actTimer)
{

}

ActionTimer::~ActionTimer()
{

}

Action* ActionTimer::eval(Stat * stat)
{
	memcpy (counts, stat->counts, sizeof (counts));

	for (Action *action = next; action && action != end;)
		if (action = action->eval (stat))
			return end->next;

	return next;
}

Action* ActionTimer::compile(Context * context)
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

	throw ScriptError ("unterminated 'timer' verb");

	return next;
}
