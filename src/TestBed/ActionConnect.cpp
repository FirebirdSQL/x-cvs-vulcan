// ActionConnect.cpp: implementation of the ActionConnect class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionConnect.h"
#include "Connection.h"
#include "Context.h"
#include "Stat.h"
#include "ActionStatement.h"

#define LOWER(c)		((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c)

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionConnect::ActionConnect() : Action (actConnect)
{
	connection = NULL;
	statements = NULL;
	autoCommit = false;
}

ActionConnect::~ActionConnect()
{
	FOR_OBJECTS (char*, cookie, &cookies)
		delete [] cookie;
	END_FOR;

	close();
}

Action* ActionConnect::eval(Stat *stat)
{
	if (connection)
		close();

	const char *db = getStringArg ("db", "");
	const char *account = getStringArg ("account", "");
	const char *passwd = getStringArg ("passwd", "");

	connection = createConnection();
	Properties *properties = connection->allocProperties();
	properties->putValue ("user", account);
	properties->putValue ("password", passwd);
	connection->openDatabase (db, properties);
	delete properties;

	connection->setAutoCommit(autoCommit);
	const char *nameSpace = getStringArg ("namespace", NULL);

	if (nameSpace)
		{
		JString sql;
		sql.Format ("alter namespace push %s", nameSpace);
		Statement *statement = connection->createStatement();
		statement->execute (sql);
		statement->close();
		}

	INCR (stat, stat_connection);
	INCR (stat, stat_any);

	return next;
}

void ActionConnect::close()
{
	for (ActionStatement *statement = statements; statement; statement = statement->nextStatement)
		statement->close();

	if (connection)
		{
		connection->close();
		connection = NULL;
		}
}

Action* ActionConnect::compile(Context * context)
{
	context->push (this);
	autoCommit = getBoolArg("autocommit", false);

	return next;
}

void ActionConnect::fini()
{
	close();
}

void ActionConnect::processCookies(const char *html)
{
	// Look for cookies in the generated string

	for (const char *p = html; *p && *p != '\r';)
		{
		for (const char *q = "set-cookie:"; *p && *q && LOWER (*p) == *q; ++p, ++q)
			;
		if (!*q)
			{
			while (*p == ' ')
				++p;
			const char *start = p;
			while (*p && *p != ' ' && *p != '\r' && *p != ';')
				++p;
			int len = p - start;
			char *cookie = new char [len + 1];
			memcpy (cookie, start, len);
			cookie [len] = 0;
			cookies.append (cookie);
			}
		while (*p && *p++ != '\n')
			;
		}
}

JString ActionConnect::getCookies()
{
	char temp [1024], *t = temp;
	*t = 0;

	// Handle any cookies present

	FOR_OBJECTS (const char*, q, &cookies)
		while (*q)
			*t++ = *q++;
		*t++ = ';';
		*t++ = ' ';
		*t = 0;
	END_FOR;

	return temp;
}

void ActionConnect::addStatement(ActionStatement *statement)
{
	statement->nextStatement = statements;
	statements = statement;
}
