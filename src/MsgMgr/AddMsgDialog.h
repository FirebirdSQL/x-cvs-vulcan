#pragma once
#include "afxwin.h"

class Database;

// AddMsgDialog dialog

class AddMsgDialog : public CDialog
{
	DECLARE_DYNAMIC(AddMsgDialog)

public:
	AddMsgDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~AddMsgDialog();

// Dialog Data
	enum { IDD = IDD_ADD_MSG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CComboBox facilities;
	CString facility;
	void populate(Database* db);
	
	Database	*database;
	virtual BOOL OnInitDialog(void);
};
