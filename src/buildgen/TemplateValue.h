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
 
// TemplateValue.h: interface for the TemplateValue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEMPLATEVALUE_H__8581BF57_9ACD_4A0D_9F16_2EA2A50353A8__INCLUDED_)
#define AFX_TEMPLATEVALUE_H__8581BF57_9ACD_4A0D_9F16_2EA2A50353A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

class TemplateValue  
{
public:
	TemplateValue(JString attName, JString attValue, int level, int hashSize);
	virtual ~TemplateValue();

	JString			name;
	JString			value;
	TemplateValue	*collision;
	TemplateValue	*stack;
	int				level;
	int				slot;
};

#endif // !defined(AFX_TEMPLATEVALUE_H__8581BF57_9ACD_4A0D_9F16_2EA2A50353A8__INCLUDED_)
