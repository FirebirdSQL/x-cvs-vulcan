/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			SQLError.cpp
 *	DESCRIPTION:	SQL Exception object
 *
 * copyright (c) 1997 - 2000 by James A. Starkey
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "Engine.h"
#include "SQLError.h"
#include "Stream.h"

#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


SQLError::SQLError (SqlCode code, const char *txt, ...)
{
/**************************************
 *
 *		S Q L E x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *		SQL exception -- quite generic.
 *
 **************************************/
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	stackTrace = NULL;
	useCount = 0;

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	va_end(args);
	text = temp;
	sqlcode = (int) code;
}

SQLError::SQLError(Stream * trace, SqlCode code, const char * txt, ...)
{
/**************************************
 *
 *		S Q L E x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *		SQL exception -- quite generic.
 *
 **************************************/
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	useCount = 0;
	int length = trace->getLength();
	char *buffer = stackTrace.getBuffer (length + 1);
	trace->getSegment (0, length, buffer);
	buffer [length] = 0;
	stackTrace.releaseBuffer();

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	va_end(args);
	text = temp;
	sqlcode = (int) code;
}

SQLError::~SQLError ()
{
/**************************************
 *
 *		~ S Q L E x c e p t i o n
 *
 **************************************
 *
 * Functional description
 *		Object destructor.
 *
 **************************************/

}

int SQLError::getSqlcode ()
{
/**************************************
 *
 *		g e t S q l c o d e
 *
 **************************************
 *
 * Functional description
 *		Get standard sql code.
 *
 **************************************/

return sqlcode;
}

const char *SQLError::getText ()
{
/**************************************
 *
 *		g e t T e x t
 *
 **************************************
 *
 * Functional description
 *		Get text of exception.
 *
 **************************************/

return text;
}

SQLError::operator const char* ()
{
/**************************************
 *
 *		o p e r a t o r   c h a r *
 *
 **************************************
 *
 * Functional description
 *		Return string as string.
 *
 **************************************/

return getText();
}

SQLError::SQLError(int code, const char * txt, ...)
{
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	useCount = 0;
	stackTrace = NULL;

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	va_end(args);
	text = temp;
	sqlcode = (int) code;
}

const char* SQLError::getTrace()
{
	return stackTrace;
}

void SQLError::addRef()
{
	++useCount;
}

int SQLError::release()
{
	//ASSERT (useCount > 0);

	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}
