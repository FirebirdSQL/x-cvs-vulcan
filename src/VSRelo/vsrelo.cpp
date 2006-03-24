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

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#endif

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

#define HELP_TEXT	"Need help? Try this:\n"

static const Switches switches [] =
	{
	"",		NULL,		&input,	"vcproj-file",	NULL,
	"-o",	NULL,		&output,"filename",	"Build directory",
	"-s",	&swSolution,NULL,	NULL,		"Do whole solution",
	"-c",	&swCreate,	NULL,	NULL,		"Create project directories",
	"-h",	&swHelp,	NULL,	NULL,		"Print this text",
	NULL
	};

int main (int argc, const char **argv)
{
	Args args;
	args.parse(switches, argc, argv);

	if (swHelp)
		{
		Args::printHelp (HELP_TEXT, switches);
		return 0;
		}

#ifdef _WIN32
	//Need to add the requisite error checking.
	CreateDirectory(output, NULL);
#else	
	if (mkdir(output, 0755) < 0)
		{
		if ( errno != EEXIST )
			{
			if ( errno == ENOENT )
				fprintf( stderr, "Some part of the path %s does not exist.\n", output );
			else
				fprintf (stderr, "Failed to create %s\n", output);
			exit( 1 );
			}
		}
#endif

	if (swSolution)
		{
		ScanDir scan(input, "*");

		while (scan.next())
			if (scan.isDirectory() && !scan.isDots())
				{
				JString projectFile;
				const char *filePath = scan.getFilePath();
				const char *fileName = scan.getFileName();
//				projectFile.Format("%s\\%s.vcproj", filePath, fileName);
#ifdef _WIN32
				projectFile.Format("%s\\%s.vcproj", filePath, fileName);
#else
				projectFile.Format("%s/%s.vcproj", filePath, fileName);
#endif				
				try
					{
					JString inputPath = PathName::expandFilename(projectFile);
					Relo relo(inputPath);

					if (swCreate)
						{
						JString dir;
						
#ifdef _WIN32
//Need to add relevant error checking for win32
						dir.Format("%s\\%s", output, fileName);
						CreateDirectory(dir, NULL);
#else
						dir.Format("%s/%s", output, fileName);
						if (mkdir((const char*) dir, 0755) < 0)
							{
							if ( errno != EEXIST )
								{
								if ( errno == ENOENT )
    	            				fprintf( stderr, "Some part of the path %s does not exist.\n", (const char*) dir );
            					else
									fprintf (stderr, "Failed to create %s\n", (const char*) dir);
					         	exit( 1 );
								}
							}
#endif
						
						}

					JString outputPath;
#ifdef _WIN32
					outputPath.Format("%s\\%s\\", output, (const char*) relo.component);
#else
					outputPath.Format("%s/%s/", output, (const char*) relo.component);
#endif
					relo.rewrite(outputPath);
					}
				catch (AdminException& exception)
					{
					fprintf (stderr, "\t%s\n", exception.getText());
					}
				}
		}
	else
		{
		JString inputPath = PathName::expandFilename(input);
		Relo relo(inputPath);
		JString outputPath;
#ifdef _WIN32
		outputPath.Format("%s\\%s\\", output, (const char*) relo.component);
#else
		outputPath.Format("%s/%s/", output, (const char*) relo.component);
#endif
		
		if (swCreate)
			{
			JString dir;
#ifdef _WIN32
			dir.Format("%s\\%s", output, relo.component);
//need to print relevant error if appropriate.			
			CreateDirectory(dir, NULL);
#else
			dir.Format("%s/%s", output, (const char*) relo.component);
			if ( mkdir((const char*) dir, 0755)== -1 )
				fprintf (stderr, "Failed to create %s\n", (const char*) dir);
#endif
			}
						
		relo.rewrite(outputPath);
		}

	return 0;
}
