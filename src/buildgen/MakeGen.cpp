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

// MakeGen.cpp: implementation of the MakeGen class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include "BuildGen.h"
#include "MakeGen.h"
#include "Args.h"
#include "XMLParse.h"
#include "InputFile.h"
#include "Element.h"
#include "AdminException.h"
#include "Stream.h"
#include "ProjectFile.h"
#include "TemplateValue.h"

static bool	printHelp;
static bool	verbose;
static char *projectFileName;
static char *configFileName = "vulcan.conf";
static char *port;
static char *component;
static char *outputFileName;

static const Switches switches [] =
	{
	WORD_ARG(projectFileName,		"projectfile")
	ARG_ARG("-f", configFileName,	"configuration file name")
	ARG_ARG("-c", component,		"component name")
	ARG_ARG("-p", port,				"port name")
	SW_ARG ("-v", verbose,			"verbose")
	ARG_ARG("-o", outputFileName,	"output file name")
	SW_ARG ("-h", printHelp,		"Print this text")
	/***
	"",		NULL,	&projectFileName,	"projectfile",	NULL,
	"-f",	NULL,	&configFileName,	NULL,		"configuration file name",
	"-c",	NULL,	&component,		NULL,			"component name",
	"-p",	NULL,	&port,			NULL,			"port name",
	"-v",	&verbose, NULL,			NULL,			"verbose",
	"-o",	NULL,	&outputFileName,NULL,			"output file name",
	"-h",	&printHelp, NULL,			NULL,		"Print this text",
	***/
	NULL
	};

#define HELP_TEXT	"Usage: buildgen -p projfile\n"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MakeGen::MakeGen()
{
	projectFiles = NULL;
	filePtr = &projectFiles;
	level = 0;
	memset (extensions, 0, sizeof (extensions));
}

MakeGen::~MakeGen()
{
	for (ProjectFile *projectFile; projectFile = projectFiles;)
		{
		projectFiles = projectFile->next;
		delete projectFile;
		}

	TemplateValue *value;

	for (int n = 0; n < hashTableSize; ++n)
		while (value = extensions [n])
			{
			extensions [n] = value->collision;
			delete value;
			}
}

int MakeGen::gen(int argc, char **argv)
{
	Args::parse (switches, argc - 1, argv + 1);

	if (printHelp)
		{
		Args::printHelp (HELP_TEXT, switches);
		return 0;
		}

	if (!projectFileName)
		{
		printf ("a project file name is required\n");
		return 1;
		}

	if (!component)
		{
		printf ("a component name is required\n");
		return 1;
		}

	try
		{
		FILE *outputFile = NULL;
		if (outputFileName)
			if (!(outputFile = fopen (outputFileName, "w")))
				throw AdminException ("can't open output file \"%s\"", outputFileName);			
		InputFile confFile (configFileName);
		XMLParse confXml;
		confXml.pushStream (&confFile);
		Element *configurations = confXml.parse();
		delete configurations;
		configurations = confXml.parseNext();

		if (verbose)
			configurations->print(0);

		Element *configuration = configurations->findChild ("configuration", "name", "Generic");

		if (configuration)
			{
			if (verbose)
				printf ("Configuration Generic\n");
			Element *comp = configuration->findChild ("component", "name", "Generic");
			if (comp)
				{
				if (verbose)
					printf ("    Component Generic\n");
				applyConfiguration (comp);
				}
			if (comp = configuration->findChild ("component", "name", component))
				{
				if (verbose)
					printf ("    Component %s\n", component);
				applyConfiguration (comp);
				}
			}

		if (configuration = configurations->findChild ("configuration", "name", port))
			{
			if (verbose)
				printf ("Configuration %s\n", port);
			Element *comp = configuration->findChild ("component", "name", "Generic");
			if (comp)
				{
				if (verbose)
					printf ("    Component %s\n", "Generic");
				applyConfiguration (comp);
				}
			if (comp = configuration->findChild ("component", "name", component))
				{
				if (verbose)
					printf ("    Component %s\n", component);
				applyConfiguration (comp);
				}
			}

		InputFile inputFile (projectFileName);
		XMLParse xmlParse;
		xmlParse.pushStream (&inputFile);
		Element *element = xmlParse.parse();
		delete element;
		element = xmlParse.parseNext();

		Element *files = element->findChild ("Files");
		
		if (!files)
			{
			element->print (0);
			throw AdminException ("can't find tag \"Files\" in file \"%s\"", projectFileName);
			}
			
		for (Element *filter = files->children; filter; filter = filter->sibling)
			if (filter->name == "Filter")
				processFilter (filter);

		Stream stream;
		expandTemplate ("Makefile", &stream);

		for (int offset = 0, len; len = stream.getSegmentLength (offset); offset += len)
			{
			const char *p = (const char*) stream.getSegment (offset);
			if (outputFile)
				fprintf (outputFile, "%.*s", len, p);
			else
				printf ("%.*s", len, p);
			}

		if (outputFile)
			fclose (outputFile);
			
		delete element;
		}
	catch (AdminException &exception)
		{
		printf ("%s, line %d: %s\n", 
				(const char*) exception.fileName, exception.lineNumber,
				exception.getText());
		return 1;
		}

	return 0;
}

void MakeGen::processConfiguration(Element *configuration)
{

}

void MakeGen::processFilter(Element *filter)
{
	const char *folder = filter->getAttributeValue ("name", NULL);

	for (Element *child = filter->children; child; child = child->sibling)
		if (child->name == "File")
			processFile (child, folder);
}

void MakeGen::processFile(Element *file, const char* folder)
{
	const char *fileName = file->getAttributeValue ("RelativePath", NULL);

	if (!fileName)
		throw AdminException ("expected relative path name for file");

	ProjectFile *projectFile = *filePtr = new ProjectFile (fileName, folder);
	filePtr = &projectFile->next;

	TemplateValue *value = findExtension (projectFile->extension);

	if (value)
		projectFile->setTargetExtension (value->value);

	Element *fileConfiguration = file->findChild ("FileConfiguration");

	if (fileConfiguration)
		{
		Element *tool = fileConfiguration->findChild ("Tool");
		if (tool)
			{
			const char *outputs = tool->getAttributeValue ("Outputs");
			if (outputs)
				projectFile->setOutputs (outputs);
			}
		}
}


void MakeGen::applyConfiguration(Element *configuration)
{
	for (Element *child = configuration->children; child; child = child->sibling)
		if (child->name == "template")
			{
			const char *name = child->getAttributeValue ("name");
			if (name)
				addTemplate (name, child);
			}
		else if (child->name == "Extension")
			{
			const char *source = child->getAttributeValue ("Source");
			const char *target = child->getAttributeValue ("Target");
			if (source && target)
				{
				TemplateValue *value = new TemplateValue (source, target, 0, hashTableSize );
				value->collision = extensions [value->slot];
				extensions [value->slot] = value;
				}
			}
}

void MakeGen::expandTag(Element *tag, Stream *stream)
{
	if (tag->name == "files")
		expandFiles (tag, stream);
	else
		expand (tag, stream);
}

void MakeGen::expandFiles(Element *tag, Stream *stream)
{
	const char *targetExtension = tag->getAttributeValue ("targetExt");
	const char *targetDir = tag->getAttributeValue ("targetDir");
	bool targetRequired = strcmp (tag->getAttributeValue ("target", ""), "Yes") == 0;

	for (ProjectFile *projectFile = projectFiles; projectFile; projectFile = projectFile->next)
		{
		if (targetExtension && projectFile->targetExtension != targetExtension)
			continue;
		if (targetRequired && projectFile->targetExtension == "")
			continue;
		int orgLevel = mark();
		push ("sourcePath", projectFile->path);
		push ("directory", projectFile->directory);
		push ("name", projectFile->name);
		push ("extension", projectFile->extension);
		push ("target", projectFile->targetExtension);
		push ("targetPath", projectFile->getTargetPath (targetDir));
		expand (tag, stream);
		revert (orgLevel);
		}

}

TemplateValue* MakeGen::findExtension(const char *extension)
{
	for (TemplateValue *value = extensions [JString::hash (extension, hashTableSize)]; value; value = value->collision)
		if (value->name == extension)
			return value;

	return NULL;
}
