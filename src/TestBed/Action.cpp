// Action.cpp: implementation of the Action class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Action.h"
#include "ArgString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Action::Action(ActionType actionType)
{
	type = actionType;
	next = NULL;
	args = NULL;
	useCount = 1;
}

Action::~Action()
{
	if (next)
		next->release();

	for (Arg *arg = args; arg = args;)
		{
		args = arg->next;
		delete arg;
		}
}

void Action::setName(CString string)
{
	name = string;
}

void Action::addArgument(CString argName, CString argValue)
{
	addArgument (new ArgString (argName, argValue));
}

void Action::addArgument(Arg * arg)
{
	arg->next = args;
	args = arg;
}

const char * Action::getStringArg(const char * argName, const char * defaultValue)
{
	for (Arg *arg = args; arg; arg = arg->next)
		if (arg->type == argString && arg->name.CompareNoCase (argName) == 0)
			return ((ArgString*) arg)->value;

	return defaultValue;
}

Action* Action::compile(Context *context)
{
	return next;
}

void Action::fini()
{

}

int Action::getIntArg(const char *argName, int defaultValue)
{
	const char *string = getStringArg (argName, NULL);

	if (!string)
		return defaultValue;

	return atoi (string);
}

void Action::addRef()
{
	++useCount;
}

void Action::release()
{
	if (--useCount == 0)
		delete this;
}

bool Action::getBoolArg(const char *argName, bool defaultValue)
{
	const char *string = getStringArg (argName, NULL);

	if (!string)
		return defaultValue;

	return stricmp (string, "yes") == 0;
}
