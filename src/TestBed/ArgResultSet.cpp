// ArgResultSet.cpp: implementation of the ArgResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ArgResultSet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArgResultSet::ArgResultSet(const char *name, ResultSet *results) : Arg (name, argResultSet)
{
	resultSet = results;
}

ArgResultSet::~ArgResultSet()
{

}
