// Arg.h: interface for the Arg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARG_H__E11366A5_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ARG_H__E11366A5_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

enum ArgType {
   argString,
   argConnection,
   argResultSet,
   argThread,
   };

class Arg  
{
public:
	Arg(const char *argName, ArgType argType);
	virtual ~Arg();

	Arg		*next;
	CString	name;
	ArgType	type;
};

#endif // !defined(AFX_ARG_H__E11366A5_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
