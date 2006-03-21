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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>
#include "fbdev.h"
#include "JString.h"

void throwSystemServicesExceptionCode(DWORD dwError = 0);
void throwSystemServicesExceptionText(const char * txt);

class SystemServicesException
{
public:
	SystemServicesException::SystemServicesException(const char * txt, ...);
	SystemServicesException(DWORD aError);
	virtual ~SystemServicesException(void);

	const char* getText();
	JString FormatErrorMessage();


private:
	DWORD dwError;
	JString	text;

};
