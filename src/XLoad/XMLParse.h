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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// XMLParse.h: interface for the XMLParse class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLPARSE_H__01A11BC4_B05D_4B33_963E_387561D7AED8__INCLUDED_)
#define AFX_XMLPARSE_H__01A11BC4_B05D_4B33_963E_387561D7AED8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Lex.h"

CLASS(InputStream);
CLASS(Element);

class XMLParse : public Lex  
{
public:
	void skipComment();
	Element* parseNext();
	Element* parse (const char *text);
	XMLParse();
	void setString (const char *stuff);
	Element* parseElement();
	Element* parse();
	XMLParse(const char *text);
	virtual ~XMLParse();

	InputStream	*stream;
	XMLParse(InputStream* stream);
	JString decodeText(Stream *stream);
};

#endif // !defined(AFX_XMLPARSE_H__01A11BC4_B05D_4B33_963E_387561D7AED8__INCLUDED_)
