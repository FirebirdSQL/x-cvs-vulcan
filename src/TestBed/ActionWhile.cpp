// ActionWhile.cpp: implementation of the ActionWhile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionWhile.h"
#include "ScriptError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionWhile::ActionWhile() : Action (actWhile)
{

}

ActionWhile::~ActionWhile()
{

}

Action* ActionWhile::eval(Stat *stat)
{
	for (;;)
		for (Action *action = next; action && action != end;)
			if (action = action->eval (stat))
				return end->next;
}

Action* ActionWhile::compile (Context *context)
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

	throw ScriptError ("unterminated 'while' verb");

	return next;
}
