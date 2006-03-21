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
 *  The Original Code was created by Paul Reeves for IBPhoenix.
 *
 *  Copyright (c) 2006 Paul Reeves
 *
 *  All Rights Reserved.
 */
 
CreateDir::CreateDir(void)
	{
	}

CreateDir::~CreateDir(void)
	{
	}

CreateDir::CreateDir(const char *filename) 
	{
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

 