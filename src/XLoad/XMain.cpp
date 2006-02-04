/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by [Initial Developer's Name]
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2004 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "XDump.h"
#include "XLoad.h"
#include "Connection.h"
#include "Args.h"
#include "ArgsException.h"
#include "Stream.h"
#include "StreamSegment.h"
#include "AdminException.h"

//-d msgs.xml -s "select * from messages" msg.fdb
//-l msgs.xml msg.fdb

static bool		swHelp;
static bool		swCreate;
static bool		swVerbose;
static bool		swAll;
static const char		*databaseName;
static const char		*inputFile;
static const char		*outputFile;
static const char		*user = "sysdba";
static const char		*password = "password";
static const char		*sql;
static const char		*table;

static const Switches switches [] =
	{
	"",		NULL,		&databaseName,	"database",	NULL,
	"-c",	&swCreate,	NULL,			NULL,		"Create database",
	"-l",	NULL,		&inputFile,		NULL,		"Load XML filename",
	"-a",	&swAll,		NULL,			NULL,		"Dump all tables",
	"-s",	NULL,		&sql,			NULL,		"SQL select statement",
	"-d",	NULL,		&outputFile,	NULL,		"Dump XML filename",
	"-u",	NULL,		&user,			NULL,		"User name",
	"-p",	NULL,		&password,		NULL,		"Password",
	"-t",	NULL,		&table,			NULL,		"Table name",
	"-v",	&swVerbose, NULL,			NULL,		"Verbose mode",
	"-h",	&swHelp,	NULL,			NULL,		"Print this text",
	NULL
	};

static const char* HELP_TEXT =	"Usage: xload [-l xmlInput] [-d xmlOutput] databaseName\n";

static int doIt(int argc, const char** argv);

int main(int argc, const char** argv)
{
	return doIt(argc, argv);
}

int doIt (int argc, const char** argv)
{
	Args::parse (switches, argc - 1, argv + 1);

	if (swHelp)
		{
		Args::printHelp (HELP_TEXT, switches);
		return 0;
		}

	if (!databaseName)
		{
		fprintf(stderr, "database name is required\n");
		Args::printHelp (HELP_TEXT, switches);
		return 1;
		}
	
	if (outputFile)
		{
		if (!swAll && !table && !sql)
			{
			fprintf(stderr, "SQL statement required for dump\n");
			return 1;
			}
		}
	else if (!inputFile)
		{
		fprintf(stderr, "either -l or -d is required\n");
		return 1;
		}
		
	Connection * connection = NULL;
	
	try
		{
		connection = createConnection();
		Properties *properties = connection->allocProperties();
        properties->putValue (user, "sysdba");
        properties->putValue (password, "masterkey");
        properties->putValue ("client", "firebird32.dll");
		connection->openDatabase (databaseName, properties);
		connection->setAutoCommit(false);
		delete properties;
		
		if (outputFile)
			{
			XDump dump(connection);
			
			if (swAll)
				dump.dumpAllTables(true);
			else if (table)
				dump.dumpTable(table, true);
			else if (sql)
				{
				PreparedStatement *statement = connection->prepareStatement(sql);
				ResultSet *resultSet = statement->executeQuery();
				dump.dumpTable(resultSet, true);
				resultSet->close();
				statement->close();
				}
				
			Stream stream;
			dump.genXML(&stream);
			FILE *file = fopen(outputFile, "w");
			
			if (!file)
				{
				fprintf(stderr, "couldn't open file \"%s\" for write\n", outputFile);
				connection->release();
				return 1;
				}
			
			fputs ("<?xml version=\"1.0\" encoding=\"US-ASCII\"?>\n", file);

			for (Segment *segment = stream.segments; segment; segment = segment->next)
				fwrite(segment->address, 1, segment->length, file);
			
			fclose(file);
			}
		
		if (inputFile)
			{
			XLoad load(connection);
			load.loadFile(inputFile);
			load.loadMetaData();
			load.loadData();
			}
		}
	catch (SQLException& exception)
		{
		fprintf (stderr, "database error: %s\n", exception.getText());
		if (connection)
			connection->release();
		return 1;
		}
	catch (AdminException& exception)
		{
		fprintf (stderr, "xml parse error: %s\n", exception.getText());
		if (connection)
			connection->release();
		return 1;
		}
	
	connection->release();
	
	return 0;
}

