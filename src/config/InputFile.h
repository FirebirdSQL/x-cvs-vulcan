// InputFile.h: interface for the InputFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INPUTFILE_H__8FAAB146_720E_43EE_A79B_262761DD0921__INCLUDED_)
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

#define AFX_INPUTFILE_H__8FAAB146_720E_43EE_A79B_262761DD0921__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "InputStream.h"
#include "JString.h"

#define AT_FILE_END		100000

START_NAMESPACE

struct FileChange {
	FileChange	*next;
	int			lineNumber;
	int			linesSkipped;
	JString		insertion;
};

class InputFile : public InputStream  
{
public:
	static bool pathEqual(const char *path1, const char *path2);
	void rewrite();
	void postChange (int lineNumber, int skip, JString insertion);
	virtual InputFile* getInputFile();
	virtual const char* getFileName();
	virtual const char* getSegment();
	virtual void close();
	InputFile(const char *fileName);
	virtual ~InputFile();

	JString		fileName;
	void		*file;
	char		buffer [1024];
	FileChange	*changes;
	InputFile(void);
	bool openInputFile(const char* fileName);
};

END_NAMESPACE

#endif // !defined(AFX_INPUTFILE_H__8FAAB146_720E_43EE_A79B_262761DD0921__INCLUDED_)
