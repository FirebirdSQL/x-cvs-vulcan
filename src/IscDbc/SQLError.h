/*
 *	PROGRAM:		Virtual Data Manager
 *	MODULE:			SQLError.h
 *	DESCRIPTION:	SQL Exception object
 *
 * copyright (c) 1997 - 2000 by James A. Starkey
 */

#ifndef __SQLERROR_H
#define __SQLERROR_H

#include "SQLException.h"
#include "JString.h"

class Stream;

class SQLError : public SQLException
{
public:
	virtual int release();
	virtual void addRef();
	virtual const char* getTrace();
	SQLError (int sqlcode, const char *text, ...);
	SQLError (SqlCode sqlcode, const char *text, ...);
	SQLError (Stream *trace, SqlCode code, const char *txt,...);
	virtual ~SQLError();

	virtual int			getSqlcode ();
	virtual const char	*getText();

	//void		Delete();
	operator	const char*();

	int		sqlcode;
	JString	text;
	JString	stackTrace;
	int		useCount;
    };

#endif
