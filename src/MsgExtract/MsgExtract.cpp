#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fbdev.h"
#include "MsgExtract.h"
#include "Connection.h"
#include "PStatement.h"
#include "RSet.h"
#include "Types.h"

// -d firefox:messages -u firebird -p public -r READER

//INSERT INTO LOCALES (LOCALE, DESCRIPTION) VALUES ('pg_PG', 'Pig Latin, testing locale only.');
//INSERT INTO FACILITIES (LAST_CHANGE, FACILITY, FAC_CODE, MAX_NUMBER) VALUES ('2005-10-20 00:13:44', 'JRD', 0, 540);
//INSERT INTO HISTORY (CHANGE_NUMBER, CHANGE_WHO, CHANGE_DATE, FAC_CODE, NUMBER, OLD_TEXT, OLD_ACTION, OLD_EXPLANATION, LOCALE) VALUES (1, 'sriram', '1993-06-14 10:51:23', 16, 5, 'enter file size or <Ctrl-D> to end input', NULL, NULL, 'c_pg');
//INSERT INTO MESSAGES (SYMBOL, ROUTINE, MODULE, TRANS_NOTES, FAC_CODE, NUMBER, FLAGS, TEXT, "ACTION", EXPLANATION) VALUES ('', 'process_statement', 'dtr.c', NULL, 1, 351, NULL, 'Do you want to roll back your updates?', NULL, NULL);

struct Table {
	const char	*name;
	const char	*order;
	const char	**fields;
	const char	*filename;
	};

static const char *localeFields[] = { "LOCALE", "DESCRIPTION", NULL };
static const char *facilitiesFields[] = { "LAST_CHANGE", "FACILITY", "FAC_CODE", "MAX_NUMBER", NULL };
static const char *msgsFields[] = { "SYMBOL", "ROUTINE", "MODULE", "TRANS_NOTES", "FAC_CODE", "NUMBER", "FLAGS", "TEXT", "\"ACTION\"", "EXPLANATION", NULL };

static const Table tables [] = {
	"LOCALES", NULL, localeFields, "locales.sql",
	"FACILITIES", "FAC_CODE", facilitiesFields, "facilities.sql",
	//"MESSAGES", "FAC_CODE, NUMBER", msgsFields, "messages.sql",
	"MESSAGES", NULL, msgsFields, "messages.sql",
	NULL
	};

MsgExtract::MsgExtract(int argc, const char **argv)
{
	database = "msg.fdb";
	const char *account = NULL;
	const char *password = NULL;
	const char *role = NULL;

	for (const char **end = argv + argc; argv < end;)
		{
		const char *p = *argv++;
		
		if (*p == '-')
			switch (p[1])
				{
				case 'd':
					database = *argv++;
					break;
				
				case 'u':
					account = *argv++;
					break;
					
				case 'p':
					password = *argv++;
					break;
				
				case 'r':
					role = *argv++;
					break;
				}
		}

	connection = createConnection();
	Properties *properties = connection->allocProperties();
	
	if (account)
		properties->putValue ("user", account);
	
	if (password)
		properties->putValue ("password", password);
		
	if (role)
		properties->putValue ("role", role);
		
	connection->openDatabase(database, properties);
}

MsgExtract::~MsgExtract(void)
{
	if (connection)
		connection->close();
}

void MsgExtract::genAll(void)
{
	PStatement s = connection->prepareStatement(
		"select symbol, routine from messages where symbol='exception_datatype_missalignment'");
	RSet r = s->executeQuery();
	
	while (r->next())
		printf ("%s\n", r->getString(2));
		
	bool numericType [128];
	
	for (const Table *table = tables; table->name; ++table)
		{
		char buffer [10240], *p = buffer;
		const char *sep = "select ";
		const char **field;
		
		for (field = table->fields; *field; ++field)
			{
			concat(sep, &p);
			concat(*field, &p);
			sep = ",";
			}
		
		concat(" from ", &p);
		concat(table->name, &p);
			
		if (table->order)
			{
			concat(" order by ", &p);
			concat(table->order, &p);
			}
				
		*p = 0;
		PStatement statement = connection->prepareStatement(buffer);
		RSet resultSet = statement->executeQuery();
		FILE *file = fopen(table->filename, "w");
		time_t now;
		time(&now);
		strcpy(buffer, ctime(&now));
		*strchr(buffer, '\n') = 0;
		fprintf(file, "/* Created by MsgExtract at %s from %s */\n", buffer, database);
		ResultSetMetaData *metaData = resultSet->getMetaData();
		int count = metaData->getColumnCount();
		int n;
		
		for (n = 0; n < count; ++n)
			{
			int type = metaData->getColumnType(n + 1);
			numericType[n] = type == SMALLINT || type == INTEGER;
			}
			
		while (resultSet->next())
			{
			p = buffer;
			concat("INSERT INTO ", &p);
			concat(table->name, &p);
			sep = " (";
			
			for (field = table->fields; *field; ++field)
				{
				concat(sep, &p);
				concat(*field, &p);
				sep = ", ";
				}
			
			sep = ") VALUES (";
			
			for (n = 0; n < count; ++n)
				{
				concat(sep, &p);
				char *start = p;
				const char *string = resultSet->getString(n + 1);

				if (strcmp(string, "exception_datatype_missalignment") == 0)
					printf ("%s\n", string);
									
				if (resultSet->wasNull())
					concat("NULL", &p);
				else if (numericType[n])
					concat(string, &p);
				else
					{
					*p++ = '\'';
					
					for (char c; c = *string++;)
						{
						*p++ = c;
						
						if (c == '\'')
							*p++ = c;
						}	
				
					while (p > start && (p[-1] == '\n' || p[-1] == ' '))
						--p;
						
					*p++ = '\'';
					}
			
				sep = ", ";
				}
			
			concat(");\n", &p);
			*p = 0;
			fputs(buffer, file);
			}
		
		fprintf(file, "\nCOMMIT WORK;\n");
		fclose(file);
		}
}

void MsgExtract::concat(const char *source, char **target)
{
	char *p = *target;
	
	while (*source)
		*p++ = *source++;
	
	*target = p;
}
