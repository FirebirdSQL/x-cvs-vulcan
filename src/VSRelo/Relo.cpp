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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

// Relo.cpp: implementation of the Relo class.
//
//////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "firebird.h"
#include "Relo.h"
#include "Element.h"
#include "Stream.h"
#include "XMLParse.h"
#include "InputFile.h"
#include "PathName.h"

#define UPPER(c)	((c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c)

static const char *filterFiles [] =
	{ "VisualStudioProject", "Files", "Filter", "File", NULL };
	
static const char *filterFilterFiles [] =
	{ "VisualStudioProject", "Files", "Filter", "Filter", "File", NULL };

static const char *filterOutputs [] =
	{ "VisualStudioProject", "Files", "Filter", "Filter", "File", "FileConfiguration", "Tool", NULL };

static const char *looseFiles [] =
	{ "VisualStudioProject", "Files", "File", NULL };

static const char *tools [] =
	{ "VisualStudioProject", "Configurations", "Configuration", "Tool", NULL };

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Relo::Relo(const char *filename)
{
	const char *prior = NULL;
	const char *name = NULL;

	for (const char *p = filename; *p;)
		if (*p++ == '\\')
			{
			prior = name;
			name = p;
			}

	projectFile = name;
	component = JString(prior, name - prior - 1);
	sourceDirectory = JString(filename, name - filename);
	InputFile *stream = new InputFile (filename);
	XMLParse parse(stream);
	header = parse.parse();
	project = parse.parseNext();
}

Relo::~Relo()
{
	delete project;
}

void Relo::rewrite(const char *filename)
{
	JString path = PathName::expandFilename(filename);
	const char *p = path;
	const char *end = strrchr(p, '\\');
	targetDirectory = JString(p, end - p);

	map(opOutputs, filterOutputs, project);

	map(opRelativePath, filterFilterFiles, project);
	map(opRelativePath, filterFiles, project);
	

	map(opRelativePath, looseFiles, project);
	map(opOutputFile, tools, project);
	map(opModuleDefFile, tools, project);
	//map(opIncludeFile, tools, project);

	Stream stream;
	header->genXML(0, &stream);
	stream.putCharacter('\n');
	project->genXML(0, &stream);
	JString outputPath;
	outputPath.Format("%s\\%s", filename, (const char*) projectFile);

	FILE *file = fopen(outputPath, "w");
	
	if (!file)
		{
		fprintf(stderr, "couldn't open file \"%s\" for write\n", (const char*) outputPath);
		return;
		}

	for (Segment *segment = stream.segments; segment; segment = segment->next)
		fwrite(segment->address, 1, segment->length, file);
	
	fclose(file);
}

void Relo::map(Operation op, const char **names, Element *element)
{
	if (element->name != *names++)
		return;

	if (*names)
		for (Element *child = element->children; child; child = child->sibling)
			map(op, names, child);
	else
		switch(op)
			{
			case opRelativePath:
				mapFile("RelativePath", element);
				break;

			case opOutputFile:
				mapFile("OutputFile", element);
				break;

			case opOutputs:
				mapFile("Outputs", element);
				break;

			case opModuleDefFile:
				mapFile("ModuleDefinitionFile", element);
				mapFile("ImportLibrary", element);
				mapFile("AdditionalLibraryDirectories", element);
				mapFileList("AdditionalIncludeDirectories", element);
				break;

			case opIncludeFile:
				mapFileList("AdditionalIncludeDirectories", element);
				break;
			}
}

void Relo::mapFile(const char *attributeName, Element *element)
{
	Element *attribute = element->findAttribute(attributeName);

	if (!attribute)
		return;

	const char *filename = attribute->value;

	if (*filename == '$')
		return;

	JString newPath = rewriteFilename(filename);
	attribute->value = newPath;
}

JString Relo::rewriteFilename(const char *original)
{
	JString fullPath = PathName::expandFilename(original, sourceDirectory);
	const char *start1 = fullPath;
	const char *start2 = targetDirectory;
	const char *p, *q;

	for (p = start1, q = start2; *p && UPPER(*p) == UPPER(*q); ++p, ++q)
		if (*p == '\\')
			{
			start1 = p;
			start2 = q;
			}

	char buffer[256];
	char *out = buffer;

	for (q = start2; *q;)
		if (*q++ == '\\')
			{
			*out++ = '.';
			*out++ = '.';
			*out++ = '\\';
			}

	strcpy(out, start1 + 1);

	return buffer;
}

void Relo::mapFileList(const char *attributeName, Element *element)
{
	Element *attribute = element->findAttribute(attributeName);

	if (!attribute)
		return;

	char buffer [1024];
	char *out = buffer;

	for (const char *p = attribute->value; *p; ++p)
		{
		char *start = out;

		while (*p && *p != ';' && *p != ',')
			*out++ = *p++;

		*out = 0;

		if (*start == '.')
			{
			*out = 0;
			JString newPath = rewriteFilename(start);
			out = start;

			for (const char *q = newPath; *q; ++q)
				*out++ = (*q == '\\') ? '/' : *q;
			}

		*out++ = *p;
		}

	attribute->value = JString(buffer);
}
