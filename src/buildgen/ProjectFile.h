/*
 *	The contents of this file embody technology owned by
 *	Netfrastructure, Inc., and subject to United States and
 *	international patents now pending.
 *
 *	Permission is granted by Netfrastructure, Inc., to use
 *	this technology and software to build the Firebird open 
 *	source database without cost.  This permission does not 
 *	extend to any other use of this software or technology.
 *	Unlicensed use of this software or technology may subject
 *	the user to legal action.
 *
 *	This software may be distributed only with the Firebird
 *	open source database project software.  It may be reproduced 
 *	or modified subject to the restrictions listed above provided 
 *	that this notice is reproduced without change.
 *
 *	Copyright (c) 2004	Netfrastructure, Inc.
 *
 */ 

// ProjectFile.h: interface for the ProjectFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROJECTFILE_H__D39C233A_2562_4EDA_9089_03F19B3EEFCD__INCLUDED_)
#define AFX_PROJECTFILE_H__D39C233A_2562_4EDA_9089_03F19B3EEFCD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

class ProjectFile  
{
public:
	JString getTargetPath (const char *defaultDir);
	void setOutputs (const char *outputs);
	void setTargetExtension (JString extension);
	ProjectFile(const char *fileName, JString fileFolder);
	virtual ~ProjectFile();

	JString		path;
	JString		directory;
	JString		name;
	JString		extension;
	JString		targetExtension;
	JString		targetPath;
	JString		folder;
	ProjectFile	*next;
};

#endif // !defined(AFX_PROJECTFILE_H__D39C233A_2562_4EDA_9089_03F19B3EEFCD__INCLUDED_)
