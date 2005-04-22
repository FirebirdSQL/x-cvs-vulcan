/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by [Initial Developer's Name]
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2003 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */


//////////////////////////////////////////////////////////////////////

#ifndef SPECIAL_SQL_H
#define SPECIAL_SQL_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Lex.h"

class PBGen;

CLASS(Element);

class SpecialSql : public Lex
{
public:
	SpecialSql(int length, const char *sql, int userDialect);
	virtual ~SpecialSql(void);
	Element	*syntax;
	Element* parseStatement(void);
	virtual void syntaxError(const char* expected, const char * token);
	Element* parseCreateDatabase(void);
	Element* parse(void);
	int genDPB(PBGen* gen);
	JString	SpecialSql::genAlterStatement (void);
	JString SpecialSql::getCharSetName(void);
};

#endif
