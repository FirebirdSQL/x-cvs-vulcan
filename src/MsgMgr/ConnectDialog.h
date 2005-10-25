#pragma once
#include "afxwin.h"

class RecentStringList;

// ConnectDialog dialog

class ConnectDialog : public CDialog
{
	DECLARE_DYNAMIC(ConnectDialog)

public:
	ConnectDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~ConnectDialog();

// Dialog Data
	enum { IDD = IDD_CONNECT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString connectString;
	CString account;
	CString password;
	RecentStringList* hostnames;
	
	virtual BOOL OnInitDialog(void);
	virtual int DoModal(void);
	CComboBox connectStrings;
	CComboBox roles;
	CString role;
};
