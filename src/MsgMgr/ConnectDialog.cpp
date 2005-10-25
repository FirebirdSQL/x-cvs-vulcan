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
	, role(_T(""))
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
	DDX_CBString(pDX, IDC_CONNECT_STRING, connectString);
	DDX_Text(pDX, IDC_ACCOUNT, account);
	DDX_Text(pDX, IDC_PASSWORD, password);
	DDX_Control(pDX, IDC_CONNECT_STRING, connectStrings);
	DDX_Control(pDX, IDC_ROLE, roles);
	DDX_CBString(pDX, IDC_ROLE, role);
}


BEGIN_MESSAGE_MAP(ConnectDialog, CDialog)
END_MESSAGE_MAP()


// ConnectDialog message handlers

BOOL ConnectDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();

	roles.AddString("Reader");
	roles.AddString("Writer");
	role = "Reader";
	hostnames->readList();
	int count = hostnames->getSize();

	for (int n = 0; n < count; ++n)
		{
		CString host = (*hostnames)[n];
		CString accountName;
		CString roleName;
		int i = host.Find(';');
		
		if (i >= 0)
			{
			accountName = host.Mid(i + 1);
			host = host.Left(i);
			i = accountName.Find(';');
			
			if (i >= 0)
				{
				roleName = accountName.Mid(i + 1);
				accountName = accountName.Left(i);
				}
			}
		
		if (host != "")
			{
			connectStrings.AddString (host);
			
			if (connectString.IsEmpty())
				{
				connectString = host;
				role = roleName;
				account = accountName;
				}
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
		CString string = connectString + ';' + account + ';' + role;
		hostnames->add (string);
		hostnames->writeList();
		}

	return ret;
}
