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

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "Args.h"
#include "Relo.h"
#include "PathName.h"
#include "ScanDir.h"
#include "AdminException.h"

static const char *input;
static const char *output;
static bool	swHelp;
static bool	swSolution;
static bool	swCreate;

static const Switches switches [] =
	{
	"",		NULL,		&input,	"vcproj-file",	NULL,
	"-o",	NULL,		&output,"filename",	"Build directory",
	"-s",	&swSolution,NULL,	NULL,		"Do whole solution",
	"-c",	&swCreate,	NULL,	NULL,		"Create project directories",
	"-h",	&swHelp,	NULL,	NULL,		"Print this text",
	NULL
	};

main (int argc, const char **argv)
{
	Args args;
	args.parse(switches, argc, argv);

	if (swSolution)
		{
		ScanDir scan(input, "*");

		while (scan.next())
			if (scan.isDirectory() && !scan.isDots())
				{
				JString projectFile;
				const char *filePath = scan.getFilePath();
				const char *fileName = scan.getFileName();
				projectFile.Format("%s\\%s.vcproj", filePath, fileName);

				try
					{
					JString inputPath = PathName::expandFilename(projectFile);
					Relo relo(inputPath);

					if (swCreate)
						{
						JString dir;
						dir.Format("%s\\%s", output, fileName);
						CreateDirectory(dir, NULL);
						}

					JString outputPath;
					outputPath.Format("%s\\%s\\", output, (const char*) relo.component);
					relo.rewrite(outputPath);
					}
				catch (AdminException& exception)
					{
					fprintf (stderr, "%s\n", exception.getText());
					}
				}
		}
	else
		{
		JString inputPath = PathName::expandFilename(input);
		Relo relo(inputPath);
		JString outputPath;
		outputPath.Format("%s\\%s\\", output, (const char*) relo.component);
		
		if (swCreate)
			{
			JString dir;
			dir.Format("%s\\%s", output, relo.component);
			CreateDirectory(dir, NULL);
			}
						
		relo.rewrite(outputPath);
		}

	return 0;
}