// ListMsgsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "ListMsgsDialog.h"
#include "PStatement.h"
#include "RSet.h"
#include "Database.h"


// ListMsgsDialog dialog

IMPLEMENT_DYNAMIC(ListMsgsDialog, CDialog)
ListMsgsDialog::ListMsgsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ListMsgsDialog::IDD, pParent)
	, facility(_T(""))
	, containing(_T(""))
{
	database = NULL;
}

ListMsgsDialog::~ListMsgsDialog()
{
}

void ListMsgsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FACILITY, facilities);
	DDX_CBString(pDX, IDC_FACILITY, facility);
	DDX_Text(pDX, IDC_CONTAINING, containing);
}


BEGIN_MESSAGE_MAP(ListMsgsDialog, CDialog)
	ON_BN_CLICKED(IDC_ORDER_NUMBER, OnBnClickedOrderNumber)
	ON_BN_CLICKED(IDC_ORDER_SYMBOL, OnBnClickedOrderSymbol)
	ON_BN_CLICKED(IDC_ORDER_TEXT, OnBnClickedOrderText)
END_MESSAGE_MAP()


// ListMsgsDialog message handlers

BOOL ListMsgsDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();
	PStatement statement = database->connection->prepareStatement(
		"select facility from facilities order by facility");
	RSet resultSet = statement->executeQuery();
	
	facilities.AddString(ALL);
		
	if (facility.IsEmpty())
		facility = ALL;
		
	while (resultSet->next())
		{
		char temp[256];
		strcpy(temp, resultSet->getString(1));
		char *p = strchr(temp, ' ');
		
		if (p)
			*p = 0;
			
		facilities.AddString(temp);
		}
	
	order = none;
	UpdateData (false);
	
	return true;
}

void ListMsgsDialog::populate(Database* db)
{
	database = db;
}

void ListMsgsDialog::OnBnClickedOrderNumber()
{
	order = number;
}

void ListMsgsDialog::OnBnClickedOrderSymbol()
{
	order = symbol;
}

void ListMsgsDialog::OnBnClickedOrderText()
{
	order = text;
}
