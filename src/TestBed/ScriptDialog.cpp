// ScriptDialog.cpp : implementation file
//

#include "stdafx.h"
#include "TestBed.h"
#include "ScriptDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScriptDialog dialog


CScriptDialog::CScriptDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CScriptDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScriptDialog)
	m_name = _T("");
	//}}AFX_DATA_INIT
}


void CScriptDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScriptDialog)
	DDX_Text(pDX, IDC_NAME, m_name);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScriptDialog, CDialog)
	//{{AFX_MSG_MAP(CScriptDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScriptDialog message handlers
