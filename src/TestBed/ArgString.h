// ArgString.h: interface for the ArgString class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARGSTRING_H__E11366A6_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ARGSTRING_H__E11366A6_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Arg.h"

class ArgString : public Arg  
{
public:
	ArgString(const char *name, const char *string);
	virtual ~ArgString();

	CString		value;
};

#endif // !defined(AFX_ARGSTRING_H__E11366A6_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
