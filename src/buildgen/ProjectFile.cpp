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
 
// ProjectFile.cpp: implementation of the ProjectFile class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "BuildGen.h"
#include "ProjectFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ProjectFile::ProjectFile(const char *fileName, JString fileFolder)
{
	folder = fileFolder;
	next = NULL;
	char	buffer [256], *q;
	const char *p = fileName;
	
	for (q = buffer; *p;)
		{
		char c = *p++;
		if (c == '\\')
			c = '/';
		*q++ = c;
		}
	
	*q = 0;		
	path = buffer;
	const char *fileString = buffer;
	p = strrchr (fileString, '/');

	if (p)
		{
		directory.setString (fileString, p + 1 - fileString);
		fileString = p + 1;
		}

	p = strchr (fileString, '.');

	if (p)
		{
		name.setString (fileString, p - fileString);
		extension = p + 1;
		}
	else
		name = fileString;
}

ProjectFile::~ProjectFile()
{

}

void ProjectFile::setTargetExtension(JString fileExtension)
{
	targetExtension = fileExtension ;
}

void ProjectFile::setOutputs(const char *outputs)
{
	char buffer [256];
	char *q = buffer;
	char *fileExtension = NULL;
	char c;

	for (const char *p = outputs; c = *p++;)
		if (c == '$')
			{
			char subst [32], *n = subst;
			if (*p++ != '(')
				return;
			while (*p && (c = *p++) != ')')
				*n++ = c;
			*n = 0;
			const char *string = NULL;
			if (strcmp (subst, "InputDir") == 0)
				string = directory;
			else if (strcmp (subst, "InputName") == 0)
				string = name;
			if (string)
				while (*string)
					*q++ = *string++;
			}
		else
			{
			if (c == ';')
				c = ' ';
			*q++ = c;
			if (c == '.')
				fileExtension = q;
			}

	*q = 0;
	targetPath = buffer;
	
	if (fileExtension)
		targetExtension = fileExtension;
}

JString ProjectFile::getTargetPath(const char *defaultDir)
{
	if (!targetPath.IsEmpty())
		return targetPath;

	JString pathName;

	if (defaultDir)
		pathName += defaultDir;

	pathName += name;
	pathName += ".";
	pathName += targetExtension;

	return pathName;
}
