#ifndef __ISCDBC_H_
#define __ISCDBC_H_

#include <ibase.h>
#include "JString.h"

#ifndef NULL
#define NULL		0
#endif

#define SQLEXCEPTION		SQLError
#define NOT_YET_IMPLEMENTED	throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "not yet implemented")
#define THROW_ISC_EXCEPTION(statusVector)			throw SQLEXCEPTION (statusVector [1], IscConnection::getIscStatusText (statusVector))
#define ROUNDUP(a,b)		((a + b - 1) / b * b)
#define MIN(a,b)			(((a) < (b)) ? (a) : (b))
#define MAX(a,b)			(((a) > (b)) ? (a) : (b))

#ifdef _WIN32

#define strcasecmp		stricmp
#define strncasecmp		strnicmp

#else

#define __int64			long long
#define _stdcall
#endif

typedef unsigned char	UCHAR;
//typedef unsigned long	ULONG;
typedef __int64			QUAD;
typedef unsigned __int64			UQUAD;

/*
 *		Sql types (actually from java.sql.types)
 */

#include "JavaType.h"

#endif
