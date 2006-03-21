#pragma once


// AddFacilityDialog dialog

class AddFacilityDialog : public CDialog
{
	DECLARE_DYNAMIC(AddFacilityDialog)

public:
	AddFacilityDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~AddFacilityDialog();

// Dialog Data
	enum { IDD = IDD_ADD_FACILITY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString facility;
	CString facCode;
};
