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
 
// Template.h: interface for the Template class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEMPLATE_H__EA01DA24_4B06_4D11_AB9F_3BAFF3279D8D__INCLUDED_)
#define AFX_TEMPLATE_H__EA01DA24_4B06_4D11_AB9F_3BAFF3279D8D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "JString.h"

class Template  
{
public:
	Template(JString templateName, Element *pTemplate);
	virtual ~Template();

	JString		name;
	Element		*element;
	Template	*collision;
};

#endif // !defined(AFX_TEMPLATE_H__EA01DA24_4B06_4D11_AB9F_3BAFF3279D8D__INCLUDED_)
