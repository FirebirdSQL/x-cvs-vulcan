#if !defined(AFX_SCRIPTDIALOG_H__B96C96D9_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_SCRIPTDIALOG_H__B96C96D9_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ScriptDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScriptDialog dialog

class CScriptDialog : public CDialog
{
// Construction
public:
	CScriptDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CScriptDialog)
	enum { IDD = IDD_SCRIPT_DIALOG };
	CString	m_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScriptDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScriptDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCRIPTDIALOG_H__B96C96D9_0584_11D4_98DB_0000C01D2301__INCLUDED_)
