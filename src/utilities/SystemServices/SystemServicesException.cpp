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


#include "SystemServicesException.h"

#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif


void throwSystemServicesExceptionCode(DWORD dwError)
{
	if (dwError == 0)
		dwError = ::GetLastError();

	SystemServicesException* E = new SystemServicesException( dwError );
	throw E;
}

void throwSystemServicesExceptionText(const char * txt)
{
	if ( txt == "" )
		txt = "An unknown error occurred.";

	SystemServicesException* E = new SystemServicesException( txt );
	throw E;
}

SystemServicesException::SystemServicesException(const char * txt, ...)
{
	va_list		args;
	va_start	(args, txt);
	char		temp [1024];

	if (vsnprintf (temp, sizeof (temp) - 1, txt, args) < 0)
		temp [sizeof (temp) - 1] = 0;

	va_end(args);
	text = temp;
}


SystemServicesException::SystemServicesException(DWORD aError)
{
	dwError = aError;

	LPVOID lpMsgBuf;
	FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        dwError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	text.Format("Error code %d: %s", dwError, lpMsgBuf);
}

SystemServicesException::~SystemServicesException(void)
{

}


const char* SystemServicesException::getText()
{
	return text;
}
