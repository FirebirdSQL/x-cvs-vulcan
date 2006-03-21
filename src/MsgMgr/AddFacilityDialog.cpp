// AddFacilityDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "AddFacilityDialog.h"


// AddFacilityDialog dialog

IMPLEMENT_DYNAMIC(AddFacilityDialog, CDialog)
AddFacilityDialog::AddFacilityDialog(CWnd* pParent /*=NULL*/)
	: CDialog(AddFacilityDialog::IDD, pParent)
	, facility(_T(""))
	, facCode(_T(""))
{
}

AddFacilityDialog::~AddFacilityDialog()
{
}

void AddFacilityDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_FACILITY, facility);
	DDX_Text(pDX, IDC_FAC_CODE, facCode);
}


BEGIN_MESSAGE_MAP(AddFacilityDialog, CDialog)
END_MESSAGE_MAP()


// AddFacilityDialog message handlers
