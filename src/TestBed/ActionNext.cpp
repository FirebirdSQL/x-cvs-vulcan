// ActionNext.cpp: implementation of the ActionNext class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionNext.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionNext::ActionNext() : Action (actNext)
{

}

ActionNext::~ActionNext()
{

}

Action* ActionNext::eval(Stat *stat)
{
	return next;
}
