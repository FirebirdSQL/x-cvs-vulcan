// TemplateExpansion.h: interface for the TemplateExpansion class.
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

//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEMPLATEEXPANSION_H__9E6F627F_BCA5_4504_8B58_220FC55422D0__INCLUDED_)
#define AFX_TEMPLATEEXPANSION_H__9E6F627F_BCA5_4504_8B58_220FC55422D0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

static const int hashTableSize =	101;

class Template;
class TemplateValue;


class TemplateExpansion  
{
public:
	TemplateValue* findValue (const char *name);
	void revert (int orgLevel);
	void push (JString name, JString value);
	int mark();
	JString expandTemplate (const char *templateName);
	void expandTemplate (const char *templateName, Stream *stream);
	virtual void expandTag (Element *tag, Stream *stream);
	virtual void expandText (const char *text, Stream *stream);
	virtual void expand (Element *element, Stream *stream);
	virtual Element* findTemplate (const char *name);
	virtual void addTemplate (JString name, Element *element);
	TemplateExpansion();
	virtual ~TemplateExpansion();

	Template		*hashTable [hashTableSize];
	TemplateValue	*values [hashTableSize];
	TemplateValue	*valueStack;
	int				level;
};

#endif // !defined(AFX_TEMPLATEEXPANSION_H__9E6F627F_BCA5_4504_8B58_220FC55422D0__INCLUDED_)
