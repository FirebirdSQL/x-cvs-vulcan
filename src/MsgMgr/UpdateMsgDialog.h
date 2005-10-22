#pragma once
#include "afxwin.h"

class Database;

// UpdateMsgDialog dialog

class UpdateMsgDialog : public CDialog
{
	DECLARE_DYNAMIC(UpdateMsgDialog)

public:
	UpdateMsgDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~UpdateMsgDialog();

// Dialog Data
	enum { IDD = IDD_UPDATE_MSG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox facilities;
	CString facility;
	CString number;
	CString text;
	CString symbol;
	CString explanation;
	CString notes;
	
	Database	*database;
	afx_msg void OnBnClickedFind();
	virtual BOOL OnInitDialog(void);
};
