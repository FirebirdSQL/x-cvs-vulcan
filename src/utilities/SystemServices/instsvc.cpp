/*
 *  
 *	The contents of this file are subject to the Initial 
 *  Developer's Public License Version 1.0 (the "License"); 
 *  you may not use this file except in compliance with the 
 *  License. You may obtain a copy of the License at 
 *    http://www.ibphoenix.com/idpl.html. 
 *
 *  Software distributed under the License is distributed on 
 *  an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *  express or implied.  See the License for the specific 
 *  language governing rights and limitations under the License.
 *
 *  The contents of this file or any work derived from this file
 *  may not be distributed under any other license whatsoever 
 *  without the express prior written permission of the original 
 *  author.
 *
 *  The Original Code was created by Paul Reeves for the Firebird Project
 *  Copyright (c) 2006 Paul Reeves
 *  All Rights Reserved.
 *
 *  Contributor(s): ______________________________________.
 *
 */

#pragma once

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "license.h"
#include "Args.h"
#include "SystemServices.h"

static bool	swHelp;
static bool	swInstall;
static bool	swManual;
static bool	swDesktop;
static bool	swQuery;
static bool	swRemove;	
static bool	swStart;	
static bool	swStop;
static bool swTest;
static bool	swVerbose;
static bool	swVersion;
static const char *uname;
static const char *pword;


#define HELP_TEXT	"help on instsvc usage... \n"

#define MORE_HELP	"  This utility should be located and run from the 'bin' \n \
  directory of your Firebird installation. \n \
  more help stuff might go here. \n"

/* This is the definition of Switches from Args.h
struct Switches 
	{
    const char	*string;
	bool		*boolean;
	const char	**argument;
	const char	*argName;
	const char	*description;
	};
*/
static const Switches switches [] =
	{
	"",		NULL,		NULL,   NULL,		"",
	"Service Installation",		NULL,		NULL,   NULL,		"",
	"-i",	&swInstall,	NULL,   NULL,		"Install service",
	"-m",	&swManual,	NULL,   NULL,		"Service must be started manually",
	"-d",	&swDesktop,	NULL,   NULL,		"Service may interact with desktop",
#ifdef SERVICES_TEST_UNAME
	"-u",	NULL,		&uname,	"User Name","Name of user to run service under. User name must exist.",
	"-w",	NULL,       &pword,	"Password", "Password",
#endif
	"-r",	&swRemove,	NULL,	NULL,		"Remove service",
	"",		NULL,		NULL,   NULL,		"",
	"",		NULL,		NULL,   NULL,		"Service Control",
	"-s",	&swStart,	NULL,	NULL,		"Start service",
	"-p",	&swStop,	NULL,	NULL,		"Stop service",
	"-q",	&swQuery,	NULL,	NULL,		"Query service status",
	"",		NULL,		NULL,   NULL,		"",
	"-v",	&swVerbose,	NULL,	NULL,		"Be verbose",
	"-z",	&swVersion,	NULL,	NULL,		"Print version info",
	"-h",	&swHelp,	NULL,	NULL,		"Print this text",
	"",		NULL,		NULL,   NULL,		"",
#ifdef _DEBUG
	"-t",	&swTest,	NULL,   NULL,		"Test only. Make no changes.",
#endif
	//	"",		NULL,		NULL,   NULL,		"",
	NULL
	};


int main (int argc, const char **argv)
{
	Args args;
	args.parse(switches, argc, argv);

#ifdef _DEBUG
	swVerbose=1;
#endif

	if (swHelp || ( argc == 1 ) )
		{
		printf("instsvc version %s\n", GDS_VERSION);
		Args::printHelp (HELP_TEXT, switches);
		printf(MORE_HELP);
		return 0;
		}

	Flags flags;
	flags.action=0;
	flags.verbose=swVerbose;
//	flags.test=swTest;

	// Note - order of evaluation is important here.
	// Most SVC_nnnnnn constants are mutually exclusive
	// Exceptions are SVC_MANUAL which is ORed with SVC_INSTALL
	// and SVC_VERINFO which is ORed against any other SVC_nnnn 
	// value

	if (swInstall)
		{
		flags.action = ( SVC_INSTALL ) ;
		if (swManual)
			flags.action = ( flags.action || SVC_MANUAL );
		if (swDesktop)
			flags.action = ( flags.action || SVC_DESKTOP );
		if (uname)
			flags.uname = uname;
		if (pword)
			flags.pword = pword;

		}

	if (swRemove)
		flags.action = ( SVC_REMOVE );

	if (swStop)
		flags.action = ( SVC_STOP );

	if (swStart)
		flags.action = ( SVC_START );

	if (swQuery)
		{
		flags.action = ( SVC_STATUS );
		flags.readOnly = true;
		}

	if (swVersion || swVerbose)
		{
		printf("instsvc version %s\n", GDS_VERSION);
		}

	try
		{
		SystemServices SystemService( flags );

		if ( !SystemService.serviceSupportAvailable() )
			throwSystemServicesExceptionText( FB_NO_SERVICES );

		if ( flags.action )
			{
			if (flags.action == SVC_STATUS)
				SystemService.status();
			else
				SystemService.execute();
			}
		return 0;
		}
	catch( SystemServicesException* pSSException )
		{
		printf( pSSException->getText() );
		delete pSSException;
		return 1;
		}
}