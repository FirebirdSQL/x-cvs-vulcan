// QueryDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "QueryDialog.h"


// QueryDialog dialog

IMPLEMENT_DYNAMIC(QueryDialog, CDialog)
QueryDialog::QueryDialog(CWnd* pParent /*=NULL*/)
	: CDialog(QueryDialog::IDD, pParent)
	, query(_T(""))
{
}

QueryDialog::~QueryDialog()
{
}

void QueryDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_QUERY, query);
}


BEGIN_MESSAGE_MAP(QueryDialog, CDialog)
END_MESSAGE_MAP()


// QueryDialog message handlers
