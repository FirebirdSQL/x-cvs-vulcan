/*
 *	The contents of this file embody technology owned by
 *	Netfrastructure, Inc., and subject to United States and
 *	international patents now pending.
 *
 *	Permission is granted by Netfrastructure, Inc., to use
 *	this technology and software to build the Firebird open 
 *	source database without cost.  This permission does not 
 *	extend to any other use of this software or technology.
 *	Unlicensed use of this software or technology may subject
 *	the user to legal action.
 *
 *	This software may be distributed only with the Firebird
 *	open source database project software.  It may be reproduced 
 *	or modified subject to the restrictions listed above provided 
 *	that this notice is reproduced without change.
 *
 *	Copyright (c) 2004	Netfrastructure, Inc.
 *
 */ 
 
// TemplateExpansion.cpp: implementation of the TemplateExpansion class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "BuildGen.h"
#include "TemplateExpansion.h"
#include "Template.h"
#include "Stream.h"
#include "Element.h"
#include "AdminException.h"
#include "TemplateValue.h"

static const char *special [] =
	{
	"lt;", "<",
	"gt;", ">",
	NULL
	};
	
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TemplateExpansion::TemplateExpansion()
{
	level = 0;
	valueStack = NULL;
	memset (hashTable, 0, sizeof (hashTable));
	memset (values, 0, sizeof (values));
}

TemplateExpansion::~TemplateExpansion()
{
	Template *pTemplate;

	for (int n = 0; n < hashTableSize; ++n)
		while (pTemplate = hashTable [n])
			{
			hashTable [n] = pTemplate->collision;
			delete pTemplate;
			}
}

void TemplateExpansion::addTemplate(JString name, Element *element)
{
	Template *pTemplate = new Template (name, element);
	int slot = name.hash (hashTableSize);
	pTemplate->collision = hashTable [slot];
	hashTable [slot] = pTemplate;
}

Element* TemplateExpansion::findTemplate(const char *name)
{
	for (Template *pTemplate = hashTable [JString::hash (name, hashTableSize)];
		 pTemplate; pTemplate = pTemplate->collision)
		if (pTemplate->name == name)
			return pTemplate->element;

	return NULL;
}

void TemplateExpansion::expand(Element *element, Stream *stream)
{
	if (element->name == "value")
		{
		const char *name = element->getAttributeValue ("name");
		if (name)
			{
			TemplateValue *value = findValue (name);
			if (value)
				stream->putSegment (value->value);
			}
		return;
		}

	Element *pTemplate = findTemplate (element->name);

	if (pTemplate)
		{
		int orgLevel = mark();
		for (Element *attribute = element->attributes; attribute; attribute = attribute->sibling)
			push (attribute->name, attribute->value);			
		expand (pTemplate, stream);
		revert (orgLevel);
		return;
		}

	const char *p = element->innerText;

	if (*p == '\n')
		++p;

	expandText (p, stream);

	for (Element *child = element->children; child; child = child->sibling)
		{
		expandTag (child, stream);
		expandText (child->outerText, stream);
		}
}

void TemplateExpansion::expandText(const char *text, Stream *stream)
{
	if (!*text)
		return;
	
	const char *start = text;
	const char *p;
	
	for (p = text; *p;)
		{
		char c = *p++;
		if (c == '&')
			{
			if (p > start)
				stream->putSegment (p - start - 1, start, true);
			for (const char **spec = special; spec [0]; spec += 2)
				{
				const char *s = spec [0];
				const char *t = p;
				while (*s && *s++ == *t++)
					;
				if (!*s)
					{
					stream->putSegment (spec [1]);
					p = t;
					break;
					}
				}
			start = p;
			}
		}
		
		if (p > start)
			stream->putSegment (p - start, start, true);
}

void TemplateExpansion::expandTag(Element *tag, Stream *stream)
{
	expand (tag, stream);
}


void TemplateExpansion::expandTemplate(const char *templateName, Stream *stream)
{
	Element *pTemplate = findTemplate (templateName);

	if (!pTemplate)
		throw AdminException ("can't find template \"%s\"", templateName);

	expand (pTemplate, stream);
}

JString TemplateExpansion::expandTemplate(const char *templateName)
{
	Element *pTemplate = findTemplate (templateName);

	if (pTemplate)
		return "";

	Stream stream;
	expand (pTemplate, &stream);

	return stream.getJString();
}

int TemplateExpansion::mark()
{
	return level++;
}

void TemplateExpansion::push(JString name, JString value)
{
	TemplateValue *templateValue = new TemplateValue (name, value, level, hashTableSize);
	templateValue->stack = valueStack;
	valueStack = templateValue;
	templateValue->collision = values [templateValue->slot];
	values [templateValue->slot] = templateValue;
}

void TemplateExpansion::revert(int orgLevel)
{
	for (TemplateValue *value; (value = valueStack) && value->level > orgLevel;)
		{
		valueStack = value->stack;
		values [value->slot] = value->collision;
		delete value;
		}
}

TemplateValue* TemplateExpansion::findValue(const char *name)
{
	for (TemplateValue *value = values [JString::hash (name, hashTableSize)]; value; value = value->collision)
		if (value->name == name)
			return value;

	return NULL;
}
