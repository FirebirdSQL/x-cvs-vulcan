// ScriptError.h: interface for the ScriptError class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPTERROR_H__E11366AB_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_SCRIPTERROR_H__E11366AB_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class ScriptError  
{
public:
	const char* getText();
	ScriptError(const char *txt, ...);
	virtual ~ScriptError();

	CString text;
};

#endif // !defined(AFX_SCRIPTERROR_H__E11366AB_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
