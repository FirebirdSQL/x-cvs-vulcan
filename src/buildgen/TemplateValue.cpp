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
// TemplateValue.cpp: implementation of the TemplateValue class.
//
//////////////////////////////////////////////////////////////////////

#include "BuildGen.h"
#include "TemplateValue.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TemplateValue::TemplateValue(JString attName, JString attValue, int lvl, int hashSize)
{
	name = attName;
	value = attValue;
	level = lvl;
	slot = name.hash (hashSize);
}

TemplateValue::~TemplateValue()
{

}
