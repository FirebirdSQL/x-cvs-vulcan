// ArgString.cpp: implementation of the ArgString class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ArgString.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArgString::ArgString(const char *name, const char *string) : Arg (name, argString)
{
	value = string;
}

ArgString::~ArgString()
{

}
