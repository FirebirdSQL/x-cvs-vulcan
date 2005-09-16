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
 *  Copyright (c) 2005 James A. Starkey
 *
 *  All Rights Reserved.
 */

//////////////////////////////////////////////////////////////////////

#ifndef _EXEC_STATEMENT_H
#define _EXEC_STATEMENT_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

class Request;
class PreparedStatement;
class ResultSet;

struct jrd_nod;

class ExecStatement
{
public:
	ExecStatement(Request *request);
	~ExecStatement(void);
	
	ExecStatement	*next;
	Request			*request;
	JString			sqlString;
	PreparedStatement	*statement;
	ResultSet		*resultSet;
	
	void prepare(jrd_nod *sqlNode, bool singleton);
	void close(void);
	bool fetch(jrd_nod* valueList);
	void reset(void);
};

#endif
