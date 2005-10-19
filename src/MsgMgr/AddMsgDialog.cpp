// AddMsgDialog.cpp : implementation file
//

#include <string.h>
#include "stdafx.h"
#include "MsgMgr.h"
#include "AddMsgDialog.h"
#include "Connection.h"
#include "Database.h"


// AddMsgDialog dialog

IMPLEMENT_DYNAMIC(AddMsgDialog, CDialog)
AddMsgDialog::AddMsgDialog(CWnd* pParent /*=NULL*/)
	: CDialog(AddMsgDialog::IDD, pParent)
	, facility(_T(""))
{
	database = NULL;
}

AddMsgDialog::~AddMsgDialog()
{
}

void AddMsgDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FACILITY, facilities);
	DDX_CBString(pDX, IDC_FACILITY, facility);
}


BEGIN_MESSAGE_MAP(AddMsgDialog, CDialog)
END_MESSAGE_MAP()


// AddMsgDialog message handlers

void AddMsgDialog::populate(Database* db)
{
	database = db;
}

BOOL AddMsgDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();
	PreparedStatement *statement = database->connection->prepareStatement(
		"select facility from facilities order by facility");
	ResultSet *resultSet = statement->executeQuery();
	
	for (bool first = true; resultSet->next(); first = false)
		{
		char temp[256];
		strcpy(temp, resultSet->getString(1));
		char *p = strchr(temp, ' ');
		
		if (p)
			*p = 0;
			
		facilities.AddString(temp);
		
		if (first)
			facility = temp;
		}
	
	resultSet->close();
	statement->close();
	UpdateData (false);
	
	return true;
}
