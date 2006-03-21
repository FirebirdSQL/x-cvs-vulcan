/*
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
 *		Created August 1, 2005 by James A. Starkey
 *
 *  All Rights Reserved.
 */

#ifndef JRD_RSB_SIGNULAR_H
#define JRD_RSB_SIGNULAR_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "RecordSource.h"

class RsbSingular : public RecordSource
{
public:
	RsbSingular(CompilerScratch *csb, RecordSource *next);
	virtual ~RsbSingular(void);
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen);
	virtual void close(Request* request);
};

#endif

