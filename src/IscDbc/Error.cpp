// Error.cpp: implementation of the Error class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey

#include "Engine.h"
#include "Error.h"
#include "stdarg.h"
#include <stdio.h>
#include "SQLError.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Error::Error()
{

}

Error::~Error()
{

}

void Error::error(char * string, ...)
{
	char buffer [256];

	va_list	args;
	va_start (args, string);
	vsprintf (buffer, string, args);
	va_end(args);
	//printf ("%s\n", buffer);
	throw SQLEXCEPTION (BUG_CHECK, buffer);
}

void Error::assertionFailed(char * fileName, int line)
{
	error ("assertion failed at line %d in file %s", line, fileName);
}
