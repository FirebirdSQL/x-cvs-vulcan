// Arg.cpp: implementation of the Arg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Arg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Arg::Arg (const char *argName, ArgType argType)
{
	name = argName;
	type = argType;
}

Arg::~Arg()
{

}
