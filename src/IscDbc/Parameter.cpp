// Parameter.cpp: implementation of the Parameter class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


//#include "stdafx.h"
#include <memory.h>
#include "Parameter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Parameter::Parameter(Parameter *nxt, const char *nam, int namLen, const char *val, int valLen)
{
	next = nxt;
	nameLength = namLen;
	name = new char [nameLength + 1];
	memcpy (name, nam, nameLength);
	name [nameLength] = 0;
	valueLength = valLen;
	value = new char [valueLength + 1];
	memcpy (value, val, valueLength);
	value [valueLength] = 0;
}

Parameter::~Parameter()
{
	delete [] name;
	delete [] value;
}
