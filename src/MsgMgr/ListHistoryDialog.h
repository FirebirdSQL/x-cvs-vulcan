#pragma once
#include "afxwin.h"

enum HistoryOrder {
	hisNone,
	hisFacility,
	hisNumber,
	hisWho,
	hisDate
	};
	
class Database;

// ListHistoryDialog dialog

class ListHistoryDialog : public CDialog
{
	DECLARE_DYNAMIC(ListHistoryDialog)

public:
	ListHistoryDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ListHistoryDialog();

// Dialog Data
	enum { IDD = IDD_LIST_HISTORY };
	Database	*database;
	
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox		facilities;
	CString			facility;
	CString			number;
	HistoryOrder	order;
	
	afx_msg void OnBnClickedOrderFacility();
	afx_msg void OnBnClickedOrderDate();
	afx_msg void OnBnClickedOrderWho();
	virtual BOOL OnInitDialog(void);
	CComboBox developers;
	CString developer;
};
