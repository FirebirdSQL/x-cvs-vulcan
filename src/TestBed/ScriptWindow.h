// ScriptWindow.h: interface for the CScriptWindow class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPTWINDOW_H__E11366A2_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_SCRIPTWINDOW_H__E11366A2_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "..\WORKBENCH\Editor.h"

class CScriptWindow : public CEditor  
{
	DECLARE_DYNCREATE(CScriptWindow)
protected:
	CScriptWindow();

public:
	afx_msg void OnExecute();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScriptWindow)
	protected:
	//}}AFX_VIRTUAL

	virtual ~CScriptWindow();

	// Generated message map functions
	//{{AFX_MSG(CScriptWindow)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};

#endif // !defined(AFX_SCRIPTWINDOW_H__E11366A2_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
