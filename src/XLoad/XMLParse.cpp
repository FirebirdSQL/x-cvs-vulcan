/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

 
// XMLParse.cpp: implementation of the XMLParse class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include "firebird.h"
#include "XMLParse.h"
#include "Element.h"
#include "InputStream.h"

struct XmlEscape {
    char		character;
	char		length;
	const char	*string;
	};

#define XML_ESCAPE(c,string)	c, sizeof (string) - 1, string,

static const XmlEscape xmlEscapes [] = {
	XML_ESCAPE('&',	"amp;")
	XML_ESCAPE('\'',"apos;")
	XML_ESCAPE('"',	"quot;")
	XML_ESCAPE('<',	"lt;")
	XML_ESCAPE('>',	"gt;")
	0 };

static const char *punctuation = "<>=/";

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

XMLParse::XMLParse(const char *text) : Lex (punctuation, 0)
{
	inputStream = NULL;
	setString (text);
}


XMLParse::XMLParse() : Lex (punctuation, 0)
{
	captureStart = '>';
	captureEnd = '<';
}


XMLParse::XMLParse(InputStream* stream) : Lex (punctuation, 0)
{
	inputStream = stream;
	ptr = inputStream->getSegment();
	end = inputStream->getEnd();
}

XMLParse::~XMLParse()
{
}

Element* XMLParse::parse()
{
	getToken();

	return parseNext();
}


Element* XMLParse::parseNext()
{
	if (!match ("<"))
		syntaxError ("open angle bracket");

	return parseElement();
}

Element* XMLParse::parseElement()
{
	while (match ("!--"))
		{
		skipComment();
		if (!match ("<"))
			syntaxError ("element following comment");
		}

	JString name = getName();
	Element *element = new Element (name);
	bool hasChildren = true;
	bool isDeclaration = *(const char*) name == '?';

	while (!match (">"))
		if (match ("/"))
			hasChildren = false;
		else if (isDeclaration && match ("?"))
			hasChildren = false;
		else
			{
			JString attribute = getName();
			if (!match ("="))
				syntaxError ("=");
			if (tokenType != QUOTED_STRING)
				syntaxError ("quoted string");
			if (strchr (token, '&'))
				{
				const char *p = token;
				char *temp = new char [strlen (p) + 1];
				char *q = temp;
				for (char c; c = *p++;)
					if (c == '&')
						{
						for (const XmlEscape *escape = xmlEscapes; escape->character; ++escape)
							if (!strncmp (escape->string, p, escape->length))
								{
								*q++ = escape->character;
								p += escape->length;
								break;
								}
						}
					else
						*q++ = c;
				*q = 0;
				element->addAttribute (attribute, temp);
				delete temp;
				}
			else
				element->addAttribute (attribute, token);
			getToken();
			}

	JString text = decodeText(&stuff); //stuff.getJString();

	if (hasChildren)
		{
		element->innerText = text;
		for (;;)
			{
			if (!match ("<"))
				syntaxError ("open angle bracket");
			if (match ("/"))
				{
				if (!match (name))
					syntaxError (name);
				if (!match (">"))
					syntaxError (">");
				break;
				}
			element->addChild (parseElement());			
			}
		text = decodeText(&stuff); //stuff.getJString();
		}

	element->outerText = text;

	return element;
}

void XMLParse::setString(const char *text)
{
	if (inputStream)
		inputStream->release();

	inputStream = new InputStream (text);
	ptr = inputStream->getSegment();
	//printf ("XMLParse::setString: %x\n", ptr);
	end = inputStream->getEnd();
}

Element* XMLParse::parse(const char *text)
{
	setString (text);

	return parse();
}

void XMLParse::skipComment()
{
	while (!match ("--") || !match (">"))
		getToken();
}

JString XMLParse::decodeText(Stream *stream)
{
	JString text;
	char *q = text.getBuffer(stream->totalLength);
	
	for (Segment *segment = stream->segments; segment; segment = segment->next)
		for (const char *p = segment->address, *end = p + segment->length; p < end;)
			{
			char c = *p++;
			if (c == '&')
				for (const XmlEscape *escape = xmlEscapes; escape->character; ++escape)
					if (memcmp(p, escape->string, escape->length) == 0)
						{
						c = escape->character;
						p += escape->length;
						break;
						}
			*q++ = c;
			}
	
	*q = 0;
	text.releaseBuffer();
	
	return text;
}
