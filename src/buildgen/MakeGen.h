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

// MakeGen.h: interface for the MakeGen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAKEGEN_H__5ACD329F_A2E0_4D41_BE11_E835D5351253__INCLUDED_)
#define AFX_MAKEGEN_H__5ACD329F_A2E0_4D41_BE11_E835D5351253__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"
#include "TemplateExpansion.h"
#include "Args.h"

class ProjectFile;
class TemplateValue;

class MakeGen : public TemplateExpansion  
{
public:
	TemplateValue* findExtension(const char *extension);
	virtual void expandFiles(Element *tag, Stream *stream);
	virtual void expandTag(Element *tag, Stream *stream);
	void applyConfiguration (Element *configuration);
	Element* findConfiguration (Element *configurations, const char *name);
	void processFile (Element *file, const char* folder);
	void processFilter (Element *filter);
	void processConfiguration (Element *configuration);
	int gen (int argc, char **argv);
	MakeGen();
	virtual ~MakeGen();

	ProjectFile		*projectFiles;
	ProjectFile		**filePtr;
	TemplateValue	*extensions [hashTableSize];
	Args			args;
	void expandIf(Element* tag, Stream* stream);
	bool evalBoolean(Element* tag);
};

#endif // !defined(AFX_MAKEGEN_H__5ACD329F_A2E0_4D41_BE11_E835D5351253__INCLUDED_)
