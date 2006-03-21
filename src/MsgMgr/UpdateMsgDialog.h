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
	Database	*database;
	afx_msg void OnBnClickedFind();
	afx_msg void OnEnChangeNumber();
	virtual BOOL OnInitDialog(void);
	void messageActive(bool active);
	
	CString facility;
	CString number;
	CString text;
	CString symbol;
	CString explanation;
	CString notes;
	bool	msgActive;
	
	CComboBox facilities;
	CEdit symbolCtrl;
	CEdit textCtrl;
	CEdit explanationCtrl;
	CEdit notesCtrl;
	CButton okCtrl;
	
	afx_msg void OnCbnSelchangeFacility();
	afx_msg void OnCbnEditchangeFacility();
	afx_msg void OnCbnEditupdateFacility();
};
