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

// Relo.h: interface for the Relo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RELO_H__D2616D17_A822_4FFC_A4C1_6AE22793FAB4__INCLUDED_)
#define AFX_RELO_H__D2616D17_A822_4FFC_A4C1_6AE22793FAB4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

enum Operation {
	opRelativePath,
	opOutputFile,
	opIncludeFile,
	opModuleDefFile,
	opOutputs,
	};

//CLASS(InputStream);
CLASS(Element);

class Relo  
{
public:
	void mapFileList(const char *attributeName, Element *element);
	void mapFile (const char *attributeName, Element *element);
	JString rewriteFilename(const char *original);
	void map(Operation op, const char **nodes, Element *tree);
	void rewrite(const char *filename);
	Relo(const char *filename);
	virtual ~Relo();

	Element	*project;
	Element	*header;
	JString	sourceDirectory;
	JString	targetDirectory;
	JString	projectFile;
	JString	component;
};

#endif // !defined(AFX_RELO_H__D2616D17_A822_4FFC_A4C1_6AE22793FAB4__INCLUDED_)
