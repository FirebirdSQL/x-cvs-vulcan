// ActionStatement.cpp: implementation of the ActionStatement class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ActionStatement.h"
#include "ScriptError.h"
#include "Context.h"
#include "Connection.h"
#include "ActionConnect.h"
#include "Stat.h"
#include "SQLError.h"


int hits;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ActionStatement::ActionStatement(StatementType actionType) : Action (actStatement)
{
	type = actionType;
	statement = NULL;
	recompile = false;
}

ActionStatement::~ActionStatement()
{
}

Action* ActionStatement::compile(Context *context)
{
	if (name.IsEmpty())
		connection = (ActionConnect*) context->findAction (actConnect);
	else
		connection = (ActionConnect*) context->findAction (actConnect, name);

	if (!connection)
		throw ScriptError ("can't find connection for GenHtml verb");

	switch (type)
		{
		case Select:
		case Update:
			connection->addStatement(this);
			sql = getStringArg ("sql", NULL);
			if (!sql)
				throw ScriptError ("can't find connection for GenHtml verb");
			recompile = getBoolArg ("recompile", false);
			break;

		}
	return next;
}

Action* ActionStatement::eval(Stat *stat)
{
	switch (type)
		{
		case GenHtml:
			genHtml (stat);
			break;

		case Select:
			execSelect();
			break;

		case Update:
			execUpdate();
			break;

		case Disconnect:
			connection->close();
			break;

		case Commit:
			connection->connection->commit();
			break;

		case Rollback:
			connection->connection->rollback();
			break;

		}

	return next;
}

void ActionStatement::genHtml(Stat *stat)
{
	Properties *properties = connection->connection->allocProperties();
	const char *p;
	JString cookies = connection->getCookies();

	if (!cookies.IsEmpty())
		properties->putValue ("HTTP_COOKIE", cookies);

	if (p = getStringArg ("querystring", NULL))
		{
		CString string;
		string.Format ("%s&abc=%d", p, hits);
		properties->putValue ("query_string", string);
		//parameters.putValue ("query_string", p);
		}

	Clob *clob = connection->connection->genHTML (properties, true);
	delete properties;
	++hits;
	INCR (stat, stat_gen_html);
	INCR (stat, stat_any);

	char *html = new char [clob->length()];

	for (int offset = 0, length; length = clob->getSegmentLength (offset); offset += length)
		memcpy (html + offset, clob->getSegment (offset), length);

	connection->processCookies (html);

	if (p = getStringArg ("filename", NULL))
		{
		FILE *file = fopen (p, "w");
		fprintf (file, "%s\n%", html);
		fclose (file);
		}

	delete [] html;
}

void ActionStatement::execSelect()
{
	if (!statement)
		statement = connection->connection->prepareStatement (sql);

	ResultSet *resultSet = statement->executeQuery();
	int count = 0;

	while (resultSet->next())
		++count;

	resultSet->close();

	if (recompile)
		close();
}

void ActionStatement::execUpdate()
{
	if (!statement)
		statement = connection->connection->prepareStatement (sql);

	statement->executeUpdate();

	if (recompile)
		close();
}

void ActionStatement::close()
{
	if (statement)
		{
		statement->close();
		statement = NULL;
		}
}
