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
 *  The contents of this file or any work derived from this file
 *  may not be distributed under any other license whatsoever 
 *  without the express prior written permission of the original 
 *  author.
 *
 *  The Original Code was created by James A. Starkey
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
#include <memory.h>
#include "fbdev.h"
#include "XDump.h"
#include "Connection.h"
#include "Element.h"
#include "Stream.h"
#include "Table.h"
#include "Field.h"

static const int MAX_LINE			= 60;

static char digits [64];
static char lookup [128];

static int initialize();
static int foo = initialize();

XDump::XDump(Connection *connect)
{
	connection = connect;
	databaseMetaData = connection->getMetaData();
	tree = new Element("database");
	metaData = tree->addChild("metadata");
	data = tree->addChild("data");
	temp = NULL;
	tempLength = 0;
}

XDump::~XDump(void)
{
	delete tree;
	delete temp;
}

int initialize()
{
	int n = 0;
	int c;

	for (const char *p = "./"; *p;)
		{
		lookup [*p] = n;
		digits [n++] = *p++;
		}

	for (c = '0'; c <= '9'; ++c)
		{
		lookup [c] = n;
		digits [n++] = c;
		}

	for (c = 'A'; c <= 'Z'; ++c)
		{
		lookup [c] = n;
		digits [n++] = c;
		}

	for (c = 'a'; c <= 'z'; ++c)
		{
		lookup [c] = n;
		digits [n++] = c;
		}
	
	return 1;
}


void XDump::genXML(Stream *stream)
{
	tree->genXML(0, stream);
}

void XDump::dumpTable(ResultSet* resultSet, bool saveData)
{
	ResultSetMetaData *rsmd = resultSet->getMetaData();
	const char *tableName = rsmd->getTableName(1);
	Table tbl (databaseMetaData, tableName);
	Element *table = metaData->addChild("table");
	table->addAttribute("name", tableName);
	int count = rsmd->getColumnCount();
	const char **columns = new const char* [count + 1];
	int *types = new int [count + 1];
	
	for (int n = 1; n <= count; ++n)
		{
		columns[n] = rsmd->getColumnName(n);
		Field *field = tbl.findField(columns[n]);
		Element *column = table->addChild("column");
		column->addAttribute("name", columns[n]);
		int type = types[n] = rsmd->getColumnType(n);
		column->addAttribute("type", Field::getTypeString(type));
		
		switch (type)
			{
			case JDBC_CHAR:
			case JDBC_VARCHAR:
			case JDBC_DECIMAL:
			case JDBC_NUMERIC:
				column->addAttribute("precision", rsmd->getPrecision(n));
				break;
			}
		
		int scale = rsmd->getScale(n);
		
		if (scale)
			column->addAttribute("scale", scale);
			
		if (field && field->nullable)
			column->addAttribute("nullable", "yes");	
		}
	
	JString primaryKey = dumpPrimaryKey(table);
	dumpIndexes(table, primaryKey);
	
	if (saveData)
		{
		Element *rows = data->addChild("rows");
		rows->addAttribute("table", tableName);
		
		while (resultSet->next())
			{
			Element *row = rows->addChild("row");
			
			for (int n = 1; n < count; ++n)
				{
				int type = types[n];
				if (type == JDBC_BLOB || type == JDBC_LONGVARBINARY)
					{
					Blob *blob = resultSet->getBlob(n);
					if (!resultSet->wasNull())
						{
						int length = blob->length();
						UCHAR *bytes = new UCHAR [length];
						blob->getBytes(0, length, bytes);
						Element *column = row->addChild("column");
						column->addAttribute("name", columns[n]);
						int what = Element::analyzeData(length, bytes);
						if (what >= 0)
							column->innerText = JString((char*)bytes, length);
						else
							{
							column->addAttribute("encoding", "base64");
							column->innerText = encode64(length, bytes, MAX_LINE);
							}
						delete bytes;
						}
					blob->release();
					}
				else
					{
					const char *value = resultSet->getString(n);
					
					if (!resultSet->wasNull())
						{
						Element *column = row->addChild("column");
						column->addAttribute("name", columns[n]);
						
						switch (type)
							{
							case JDBC_CHAR:
								column->innerText = trim(value);
								break;
							
							default:
								column->innerText = value;
							}
						}
					}
				}
			}
		}
	
	delete [] columns;
	delete [] types;
}


const char* XDump::trim(const char* string)
{
	int length = strlen(string);
	const char *tail = string + length;
	
	while (tail > string && tail[-1] == ' ')
		--tail;
	
	int l = tail - string;
	
	if (l == 0)
		return "";
	
	if (l == length)
		return string;
		
	if (l + 1 > tempLength)
		{
		delete temp;
		tempLength = l + 100;
		temp = new char [tempLength];
		}
	
	memcpy(temp, string, l);
	temp[l] = 0;
	
	return temp;	
}

void XDump::dumpTable(const char* tableName, bool saveData)
{
	JString sql;
	sql.Format("select * from %s", tableName);
	PreparedStatement *statement = connection->prepareStatement(sql);
	ResultSet *resultSet = statement->executeQuery();
	dumpTable(resultSet, saveData);
	resultSet->close();
	statement->close();
}

JString XDump::dumpPrimaryKey(Element *table)
{
	const char *tableName = table->getAttributeValue("name");
	ResultSet *resultSet = databaseMetaData->getPrimaryKeys(NULL,NULL,tableName);
	Element *primaryKey = NULL;
	JString name;
	
	while (resultSet->next())
		{
		const char *columnName = resultSet->getString(4);
		
		if (!primaryKey)
			{
			primaryKey = table->addChild("primary_key");
			name = resultSet->getString(6);
			}
			
		Element *column = primaryKey->addChild("column");
		column->addAttribute("name", columnName);
		}
	
	resultSet->close();
	
	return name;	
}

void XDump::dumpIndexes(Element* table, const char *primaryKey)
{
	const char *tableName = table->getAttributeValue("name");
	ResultSet *resultSet = databaseMetaData->getIndexInfo(NULL,NULL,tableName,false,false);
	JString currentIndex;
	Element *index = NULL;
	
	while (resultSet->next())
		{
		const char *indexName = resultSet->getString(6);
		
		if (!strcmp(indexName, primaryKey))
			continue;
			
		if (!(currentIndex == indexName))
			{
			index = table->addChild("index");
			index->addAttribute("name", indexName);
			currentIndex = indexName;
			bool nonUnique = resultSet->getInt(4);
			
			if (!nonUnique)
				index->addAttribute("type", "unique");
			}
		
		const char *columnName = resultSet->getString(9);
		Element *column = index->addChild("column");
		column->addAttribute("name", columnName);
		}
	
	resultSet->close();	
}

void XDump::dumpAllTables(bool saveData)
{
	static const char *types = "TABLE";
	ResultSet *resultSet = databaseMetaData->getTables(NULL,NULL,"%", 1, &types);
	
	while (resultSet->next())
		dumpTable(resultSet->getString(3), saveData);
	
	resultSet->close();
}

JString XDump::encode64(int length, void *gook, int count)
{
	JString string;
	int max = length * 4 / 3 + 4;

	if (count)
		max += (length + count - 1) / count;

	char *out = string.getBuffer (max + 1);
	int bits = 0;
	UCHAR *p = (UCHAR*) gook, *end = p + length;
	unsigned long reg = 0;
	int run = 0;
	//*out++ = '\n';

	for (int bitsRemaining = length * 8; bitsRemaining > 0; bitsRemaining -= 6)
		{
		if (bits < 6)
			{
			reg <<= 8;
			bits += 8;
			if (p < end)
				reg |= *p++;
			}
		bits -= 6;
		*out++ = digits [(reg >> bits) & 0x3f];
		if (count && ++run == count)
			{
			*out++ = '\n';
			run = 0;
			}
		}

	int pad = length % 3;

	if (pad > 0)
		{
		*out++ = '*';
		if (pad == 1)
			*out++ = '*';
		}

	*out = 0;			
	string.releaseBuffer();
	int l = string.length();

	return string;
}
