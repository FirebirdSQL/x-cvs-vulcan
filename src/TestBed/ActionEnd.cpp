// ActionEnd.cpp: implementation of the ActionEnd class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionEnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionEnd::ActionEnd() : Action (actEnd)
{

}

ActionEnd::~ActionEnd()
{

}

Action* ActionEnd::eval(Stat *stat)
{
	return next;
}
