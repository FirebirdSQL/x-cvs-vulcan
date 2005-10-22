#pragma once
#include "afxwin.h"

class Database;

enum Order {
	none,
	number,
	symbol,
	text
	};
	
// ListMsgsDialog dialog

class ListMsgsDialog : public CDialog
{
	DECLARE_DYNAMIC(ListMsgsDialog)

public:
	ListMsgsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ListMsgsDialog();

// Dialog Data
	enum { IDD = IDD_LIST_MESSAGES };
	Database	*database;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog(void);
	void populate(Database* database);
	CComboBox facilities;
	CString facility;
	Order	order;
	afx_msg void OnBnClickedOrderNumber();
	afx_msg void OnBnClickedOrderSymbol();
	afx_msg void OnBnClickedOrderText();
};
