// ListHistoryDialog.cpp : implementation file
//

#include "stdafx.h"
#include "MsgMgr.h"
#include "ListHistoryDialog.h"
#include "PStatement.h"
#include "RSet.h"
#include "Database.h"

#define ALL		"All"

// ListHistoryDialog dialog

IMPLEMENT_DYNAMIC(ListHistoryDialog, CDialog)
ListHistoryDialog::ListHistoryDialog(CWnd* pParent /*=NULL*/)
	: CDialog(ListHistoryDialog::IDD, pParent)
	, facility(_T(""))
	, number(_T(""))
	, developer(_T(""))
{
	order = hisNone;
}

ListHistoryDialog::~ListHistoryDialog()
{
}

void ListHistoryDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FACILITY, facilities);
	DDX_CBString(pDX, IDC_FACILITY, facility);
	DDX_Text(pDX, IDC_NUMBER, number);
	DDX_Control(pDX, IDC_WHO, developers);
	DDX_CBString(pDX, IDC_WHO, developer);
}


BEGIN_MESSAGE_MAP(ListHistoryDialog, CDialog)
	ON_BN_CLICKED(IDC_ORDER_FACILITY, OnBnClickedOrderFacility)
	ON_BN_CLICKED(IDC_ORDER_DATE, OnBnClickedOrderDate)
	ON_BN_CLICKED(IDC_ORDER_WHO, OnBnClickedOrderWho)
END_MESSAGE_MAP()


// ListHistoryDialog message handlers

void ListHistoryDialog::OnBnClickedOrderFacility()
{
	order = hisFacility;
}

void ListHistoryDialog::OnBnClickedOrderDate()
{
	order = hisDate;
}

void ListHistoryDialog::OnBnClickedOrderWho()
{
	order = hisWho;
}

BOOL ListHistoryDialog::OnInitDialog(void)
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
	
	statement = database->connection->prepareStatement(
		"select distinct change_who from history");
	resultSet = statement->executeQuery();
	developers.AddString(ALL);

	if (developer.IsEmpty())
		developer = ALL;
		
	while (resultSet->next())
		{
		char temp[256];
		strcpy(temp, resultSet->getString(1));
		char *p = strchr(temp, ' ');
		
		if (p)
			*p = 0;
			
		developers.AddString(temp);
		}
	
	UpdateData (false);
	
	return true;
}
