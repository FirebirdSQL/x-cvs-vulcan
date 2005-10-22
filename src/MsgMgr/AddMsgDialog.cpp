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
	, symbol(_T(""))
	, module(_T(""))
	, routine(_T(""))
	, text(_T(""))
	, explanation(_T(""))
	, translationNotes(_T(""))
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
	DDX_Text(pDX, IDC_SYMBOL, symbol);
	DDX_Text(pDX, IDC_MODULE, module);
	DDX_Text(pDX, IDC_ROUTINE, routine);
	DDX_Text(pDX, IDC_TEXT, text);
	DDX_Text(pDX, IDC_EXPANATION, explanation);
	DDX_Text(pDX, IDC_NOTES, translationNotes);
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
	
	while (resultSet->next())
		{
		char temp[256];
		strcpy(temp, resultSet->getString(1));
		char *p = strchr(temp, ' ');
		
		if (p)
			*p = 0;
			
		facilities.AddString(temp);
		
		if (facility.IsEmpty())
			facility = temp;
		}
	
	resultSet->close();
	statement->close();
	UpdateData (false);
	
	return true;
}
