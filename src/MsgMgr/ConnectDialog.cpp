// ConnectDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "ConnectDialog.h"
#include "RecentStringList.h"

#define SECTION			"Hostnames"
#define ENTRY_FORMAT	"host%d"

// ConnectDialog dialog

IMPLEMENT_DYNAMIC(ConnectDialog, CDialog)
ConnectDialog::ConnectDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ConnectDialog::IDD, pParent)
	, connectString(_T(""))
	, account(_T(""))
	, password(_T(""))
{
	hostnames = new RecentStringList (SECTION, ENTRY_FORMAT, 10);
}

ConnectDialog::~ConnectDialog()
{
	delete hostnames;
}

void ConnectDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CONNECT_STRING, connectString);
	DDX_Text(pDX, IDC_ACCOUNT, account);
	DDX_Text(pDX, IDC_PASSWORD, password);
	DDX_Control(pDX, IDC_CONNECT_STRING, connectStrings);
}


BEGIN_MESSAGE_MAP(ConnectDialog, CDialog)
END_MESSAGE_MAP()


// ConnectDialog message handlers

BOOL ConnectDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	hostnames->readList();
	int count = hostnames->getSize();

	for (int n = 0; n < count; ++n)
		{
		CString host = (*hostnames) [n];
		if (host != "")
			{
			connectStrings.AddString (host);
			if (n == 0 && connectString == "")
				connectString = host;
			}
		}

	UpdateData (false);
	
	return true;
}

int ConnectDialog::DoModal(void)
{
	int ret = CDialog::DoModal();

	if (ret == IDOK)
		{
		hostnames->add (connectString);
		hostnames->writeList();
		}

	return ret;
}
