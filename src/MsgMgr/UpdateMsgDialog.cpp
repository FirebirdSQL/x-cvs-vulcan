// UpdateMsgDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "UpdateMsgDialog.h"
#include "PStatement.h"
#include "RSet.h"
#include "Database.h"
#include ".\updatemsgdialog.h"


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
	msgActive = true;
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
	DDX_Control(pDX, IDC_SYMBOL, symbolCtrl);
	DDX_Control(pDX, IDC_TEXT, textCtrl);
	DDX_Control(pDX, IDC_EXPANATION, explanationCtrl);
	DDX_Control(pDX, IDC_NOTES, notesCtrl);
	DDX_Control(pDX, IDOK, okCtrl);
}


BEGIN_MESSAGE_MAP(UpdateMsgDialog, CDialog)
	ON_BN_CLICKED(IDC_FIND, OnBnClickedFind)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeNumber)
	ON_CBN_SELCHANGE(IDC_FACILITY, OnCbnSelchangeFacility)
	ON_CBN_EDITCHANGE(IDC_FACILITY, OnCbnEditchangeFacility)
	ON_CBN_EDITUPDATE(IDC_FACILITY, OnCbnEditupdateFacility)
END_MESSAGE_MAP()


// UpdateMsgDialog message handlers

void UpdateMsgDialog::OnBnClickedFind()
{
	UpdateData (true);
	messageActive(true);
	
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

	messageActive(false);	
	UpdateData (false);
	
	return true;
}

void UpdateMsgDialog::OnEnChangeNumber()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	UpdateData (true);
	messageActive(false);
}

void UpdateMsgDialog::messageActive(bool active)
{
	if (msgActive == active)
		return;
	
	msgActive = active;
	okCtrl.EnableWindow(active);
	symbolCtrl.EnableWindow(active);
	textCtrl.EnableWindow(active);
	explanationCtrl.EnableWindow(active);
	notesCtrl.EnableWindow(active);
	
	if (!active)
		{
		symbol = "";
		text = "";
		explanation = "";
		notes = "";
		UpdateData (false);
		}
		
}

void UpdateMsgDialog::OnCbnSelchangeFacility()
{
	int n = facilities.GetCurSel();
	char string[128];
	int ret = facilities.GetLBText(n, string);
	UpdateData (true);
	facility = string;
	messageActive(false);
}

void UpdateMsgDialog::OnCbnEditchangeFacility()
{
}

void UpdateMsgDialog::OnCbnEditupdateFacility()
{
}
