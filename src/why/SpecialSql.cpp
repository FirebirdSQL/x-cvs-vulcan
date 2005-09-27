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
 *  Copyright (c)  2003 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

#include <stdlib.h>
#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "common.h"
#include "SpecialSql.h"
#include "Element.h"
#include "InputStream.h"
#include "OSRIException.h"
#include "PBGen.h"
#include "Stream.h"
	
SpecialSql::SpecialSql(int length, const char *sql, int userDialect) : Lex ("=()[].,;+-*/", LEX_upcase)
{
	InputStream *stream = new InputStream (sql);

	if (length)
		stream->segmentLength = length;

	pushStream (stream);
	stream->release();
	syntax = NULL;
}

SpecialSql::~SpecialSql(void)
{
	if (syntax)
		delete syntax;
}

Element* SpecialSql::parseStatement(void)
{
	if (match ("CREATE"))
		{
		if (match ("DATABASE") || match ("SCHEMA"))
			return parseCreateDatabase();
		syntaxError ("create database or schema", token);
		}
	else	
		syntaxError ("create database statement", token);
	
	return NULL;
}

void SpecialSql::syntaxError(const char* expected, const char *token)
{
	Stream error;
	error.format ("expected %s encountered %s", expected, token);
	delete syntax;
	syntax = NULL;

	throw OSRIException (isc_sqlerr, isc_arg_number, -104, 
						 isc_arg_gds, isc_no_meta_update,
						 isc_arg_gds, isc_random,
						 isc_arg_string, (const char*) error.getJString(), 0);
}

/***
statement :=
	CREATE { DATABASE | SCHEMA  } '<filespec>'
		[ USER 'username' [ PASSWORD 'password' ]]
		[ PAGE_SIZE [=] int ]
		[ LENGTH [=] int [ PAGE | PAGES ]]
		[ DEFAULT CHARACTER SET charset ]
		[ DIFFERENCE FILE '<filespec>' ]
		[ SET NAMES '?' ]
		[ <secondary_file>... ]

secondary_file :=
	FILE '<filespec>' [ <fileinfo> ]

fileinfo :-
	LENGTH [=] [ PAGE | PAGES ] | 
	STARTING [ AT [ PAGE ]] int

create database 'foo.bar' page_size=123 user 'jim' password 'xyzzy';
***/

Element* SpecialSql::parseCreateDatabase(void)
{
	Element *element = new Element ("createDatabase");
	try
		{
		if (tokenType != SINGLE_QUOTED_STRING)
			syntaxError ("quoted filename", token);
		
		element->addAttribute ("filename", token);
		int fileCount = 0;

		getToken();

		for (;;)
			if (match ("USER"))
				{
				if (tokenType != SINGLE_QUOTED_STRING)
					{
					syntaxError ("quoted username", token);
					}
					
				element->addAttribute ("user", token);
				getToken();
				}
			else if (match ("PASSWORD"))
				{
				if (tokenType != SINGLE_QUOTED_STRING)
					syntaxError ("quoted password", token);

				element->addAttribute ("password", token);
				getToken();
				}
			else if (match ("PAGE_SIZE"))
				{
				match ("=");
				int n = NUMBER;
				if (tokenType != NUMBER)
					syntaxError ("numeric page size", token);

				element->addAttribute ("page_size", token);
				getToken();
				}
			else if (match ("LENGTH"))
				{
				match ("=");
				if (tokenType != NUMBER)
					syntaxError ("numeric database size", token);

				element->addAttribute ("length", token);
				getToken();
				if (!match ("PAGE"))
					match ("PAGES");
				}
			else if (match ("SET"))
				{
				if (!match("NAMES"))
					syntaxError ("names", token);
	
				element->addAttribute ("set_names", getCharSetName());
				}				
			else if (match ("DEFAULT"))
				{
				if (!match ("CHARACTER"))
					syntaxError ("'CHARACTER'", token);
					
				if (!match ("SET"))
					syntaxError ("set", token);
					
				element->addAttribute ("charset", getCharSetName());
				}
			else if (match ("DIFFERENCES"))
				{
				if (!match ("FILE"))
					syntaxError ("'FILE'", token);
					
				if (tokenType != SINGLE_QUOTED_STRING)
					syntaxError ("quoted differences file", token);
				
				Element *child = new Element ("differences file");
				child->addAttribute ("differencesFile", token);
				getToken();
				}
			else if (match ("FILE"))
				{
				if (tokenType != SINGLE_QUOTED_STRING)
					syntaxError ("quoted name for secondary file", token);

				Element *secondaryFile = new Element("secondaryFile");
				element->addChild (secondaryFile);
				secondaryFile->addAttribute ("name", token);
				secondaryFile->addAttribute ("position", fileCount);
				getToken();

				while (true)
					{
					if (match ("STARTING"))
						{
						match ("AT");
						if (!match ("PAGE"))
							match ("PAGES");
						int n = NUMBER;
						if (tokenType != NUMBER)
							syntaxError ("numeric starting page", token);
						secondaryFile->addAttribute ("startPage", token);
						getToken();
						}
					else if (match ("LENGTH"))
						{
						match ("=");
						if (tokenType != NUMBER)
							syntaxError ("numeric database size", token);
						secondaryFile->addAttribute ("length", token);
						getToken();
						if (!match ("PAGE"))
							match ("PAGES");
						}
					else 
						break;
					}
				}
			else if (tokenType == END_OF_STREAM || isKeyword (";"))
				return element;
			else
				syntaxError ("create database option", token);
		}
	catch (OSRIException&)
		{
		delete element;
		throw;
		}
	
	return element;
}

Element* SpecialSql::parse(void)
{
	getToken();
	syntax = parseStatement();
	
	return syntax;
}

int SpecialSql::genDPB(PBGen* gen)
{
	gen->appendUCHAR (isc_dpb_version1);
	gen->putParameter(isc_dpb_SQL_dialect, SQL_DIALECT_CURRENT);
	gen->putParameter(isc_dpb_overwrite, 0);
	bool haveUser = false;
	bool havePassword = false;
	
	for (Element *attribute = syntax->attributes; attribute; attribute = attribute->sibling)
		{
		if (attribute->name == "user")
			{
			gen->putParameter(isc_dpb_user_name, (const char*) attribute->value);
			haveUser = true;
			}
		else if (attribute->name == "password")
			{
			gen->putParameter(isc_dpb_password, (const char*) attribute->value);
			havePassword = true;
			}
		else if (attribute->name == "set_names")
			gen->putParameter(isc_dpb_lc_ctype, (const char*) attribute->value);
		else if (attribute->name == "charset")
			gen->putParameter(isc_dpb_set_db_charset, (const char*) attribute->value);
		else if (attribute->name == "page_size")
			gen->putParameter(isc_dpb_page_size, atoi(attribute->value));
		}

	if (!haveUser)
		{
		const char *userName = getenv ("ISC_USER");
		if (userName  || (userName = getenv ("FB_USER")))
			gen->putParameter(isc_dpb_user_name, userName);
		}
	
	if (!havePassword)
		{
		const char *password = getenv ("ISC_PASSWORD");
		if (password || (password = getenv ("FB_PASSWORD")))
			gen->putParameter(isc_dpb_password, password);
		}

	return gen->getLength();		
}

JString	SpecialSql::genAlterStatement ()
{
	int start = 0;
	Element * attribute;
	Stream command;
	command.putSegment ("ALTER DATABASE");

	if (attribute = syntax->findAttribute("length"))
		start = atoi ((const char *)attribute->value) + 1;

	for (Element * child = syntax->children; child; child = child->sibling)
		{
		command.putSegment (" ADD");

		if (child->name == "differencesFile")
			if (attribute = child->findAttribute ("name"))
				command.format (" DIFFERENCES FILE '%s'", (const char*) attribute->value);
			else 
				syntaxError ("differences file name", attribute->value);

		else if (child->name == "secondaryFile")
			{
			if (attribute = child->findAttribute ("name"))
				command.format (" FILE '%s'", (const char*) attribute->value);
			else
				syntaxError ("secondary file name", attribute->value);

			if (attribute = child->findAttribute ("startPage"))
				{
				int startPage = atoi ((const char *) attribute->value);
				start = (startPage > start) ? startPage : start;
				}

			command.format (" STARTING %d", start);
			
			if (attribute = child->findAttribute ("length"))
				{
				command.format (" LENGTH %s", (const char*) attribute->value);
				start += atoi ((const char *)attribute->value);
				}
			}
		}

	command.putCharacter (';');

	return command.getJString();
}

/* Same as Lex::getName() but used for charset names which can be quoted */
JString SpecialSql::getCharSetName()
{
	if ((tokenType != NAME)&& (tokenType != SINGLE_QUOTED_STRING))
		syntaxError ("character set name", token);

	JString name = token;
	getToken();
	
	return name;	
}
