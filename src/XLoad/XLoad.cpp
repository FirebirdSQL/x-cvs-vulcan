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
#include <string.h>
#include "fbdev.h"
#include "XLoad.h"
#include "Connection.h"
#include "Element.h"
#include "Stream.h"
#include "XMLParse.h"
#include "InputFile.h"

XLoad::XLoad(Connection *connect)
{
	tree = NULL;
}

XLoad::~XLoad(void)
{
	delete tree;
}

void XLoad::loadFile(const char* filename)
{
	InputStream *stream = new InputFile(filename);
	load(stream);
}

void XLoad::load(InputStream* stream)
{
	XMLParse parse(stream);
	tree = parse.parse();
	
	if (tree->name == "?xml")
		tree = parse.parseNext();
	
	metaData = tree->findChild("metadata");	
	data = tree->findChild("data");	
}

void XLoad::loadMetaData(void)
{
	for (Element *element = metaData->children; element; element = element->sibling)
		if (element->name == "table")
			createTable(element);
}

void XLoad::loadData(void)
{
}

void XLoad::createTable(Element* table)
{
	Stream stream;
	stream.format("create table %s (", table->getAttributeValue("table"));
	const char *sep = "\n    ";
	
	for (Element *element = table->children; element; element = element->sibling)
		if (element->name == "column")
			{
			const char *columnName = element->getAttributeValue("name");
			const char *type = element->getAttributeValue("type");
			stream.format("%s %s %s", sep, columnName, type);
			
			if (strcmp(type, "char") == 0 || strcmp(type, "varchar") == 0)
				stream.format(" [%s]", element->getAttributeValue("precision"));
			
			sep = ",\n    ";
			}
	
	stream.putSegment(")");
	JString sql = stream.getJString();
	printf ("%s\n", sql);			
}
