// UpdateMsgDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "UpdateMsgDialog.h"
#include "PStatement.h"
#include "RSet.h"
#include "Database.h"


// UpdateMsgDialog dialog

IMPLEMENT_DYNAMIC(UpdateMsgDialog, CDialog)
UpdateMsgDialog::UpdateMsgDialog(CWnd* pParent /*=NULL*/)
	: CDialog(UpdateMsgDialog::IDD, pParent)
	, facility(_T(""))
	, number(_T(""))
	, text(_T(""))
	, symbol(_T(""))
	, explanation(_T(""))
	, notes(_T(""))
{
}

UpdateMsgDialog::~UpdateMsgDialog()
{
}

void UpdateMsgDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FACILITY, facilities);
	DDX_CBString(pDX, IDC_FACILITY, facility);
	DDX_Text(pDX, IDC_EDIT1, number);
	DDX_Text(pDX, IDC_TEXT, text);
	DDX_Text(pDX, IDC_SYMBOL, symbol);
	DDX_Text(pDX, IDC_EXPANATION, explanation);
	DDX_Text(pDX, IDC_NOTES, notes);
}


BEGIN_MESSAGE_MAP(UpdateMsgDialog, CDialog)
	ON_BN_CLICKED(IDC_FIND, OnBnClickedFind)
END_MESSAGE_MAP()


// UpdateMsgDialog message handlers

void UpdateMsgDialog::OnBnClickedFind()
{
	UpdateData (true);
	
	try
		{
		int facCode = database->getFacCode(facility);
		PStatement statement = database->connection->prepareStatement(
			"select symbol,text,explanation,trans_notes from messages "
			"  where number=? and fac_code=?");
		statement->setString(1, number);
		statement->setInt(2, facCode);
		RSet resultSet = statement->executeQuery();
		
		if (resultSet->next())
			{
			symbol = resultSet->getString("SYMBOL");
			text = resultSet->getString("TEXT");
			explanation = resultSet->getString("EXPLANATION");
			notes = resultSet->getString("TRANS_NOTES");
			}
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
			
	UpdateData (false);
}

BOOL UpdateMsgDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();
	PStatement statement = database->connection->prepareStatement(
		"select facility from facilities order by facility");
	RSet resultSet = statement->executeQuery();
	
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
	
	UpdateData (false);
	
	return true;
}
