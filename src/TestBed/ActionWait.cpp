// ActionWait.cpp: implementation of the ActionWait class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionWait.h"
#include "Context.h"
#include "ActionThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionWait::ActionWait() : Action (actWait)
{
	threads = NULL;
}

ActionWait::~ActionWait()
{
	if (threads)
		delete [] threads;
}

Action* ActionWait::eval(Stat *stat)
{
	for (int n = 0; n < count; ++n)
		threads [n]->wait();

	return next;
}

Action* ActionWait::compile(Context * context)
{
	if (threads)
		{
		delete [] threads;
		threads = NULL;
		}

	count = 0;

	for (Action *action = context->actions; action; action = action->que)
		if (action->type == actThread && action->level == context->level)
			++count;

	if (count)
		{
		ActionThread **ptr = threads = new ActionThread* [count];
		int n = 0;
		for (Action *action = context->actions; action; action = action->que)
			if (action->type == actThread && action->level == context->level)
				*ptr++ = (ActionThread*) action;
		} 

	return next;
}
