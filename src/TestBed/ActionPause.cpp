// ActionPause.cpp: implementation of the ActionPause class.
//
//////////////////////////////////////////////////////////////////////

#include <time.h>
#include <stdlib.h>
#include "stdafx.h"
#include "TestBed.h"
#include "ActionPause.h"
#include "Context.h"
#include "Stat.h"
#include "ScriptError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionPause::ActionPause() : Action (actPause)
{

}

ActionPause::~ActionPause()
{

}

Action* ActionPause::compile(Context *context)
{
	int seed = getIntArg ("seed", 0);

	if (seed)
		srand (seed);
	else
		srand( (unsigned)time( NULL ) );

	int time = getIntArg ("time", 0);

	if (time > 0)
		{
		min = time;
		max = time;
		}
	else
		{
		min = getIntArg ("min", 0);
		max = getIntArg ("max", 0);
		if (max <= min)
			throw ScriptError ("Max time must be greater than min time");
		} 

			
	return next;
}

Action* ActionPause::eval(Stat *stat)
{
	int time = max;

	if (max > min)
		time = min + (int) ((double) (rand() * (max - min))) / RAND_MAX;

	Sleep (time);

	return next;
}
