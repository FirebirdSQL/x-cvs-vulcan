// Context.cpp: implementation of the Context class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Context.h"
#include "Action.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Context::Context()
{
	actions = NULL;
	level = 0;
}

Context::~Context()
{

}

void Context::push(Action * action)
{
	action->level = level;
	action->que = actions;
	actions = action;
}

Action* Context::pop()
{
	Action *action = actions;
	actions = action->que;

	return action;
}

Action* Context::findAction(ActionType type)
{
	for (Action *action = actions; action; action = action->que)
		if (action->type == type)
			return action;
	
	return NULL;	
}

Action* Context::findAction(ActionType type, const char * name)
{
	for (Action *action = actions; action; action = action->que)
		if (action->type == type && action->name.CompareNoCase (name) == 0)
			return action;
	
	return NULL;	
}

Action* Context::mark()
{
	++level;

	return actions;
}

void Context::revert(Action * mark)
{
	while (actions && actions != mark)
		actions = actions->que;

	--level;
}
