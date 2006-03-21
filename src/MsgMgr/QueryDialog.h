#pragma once


// QueryDialog dialog

class QueryDialog : public CDialog
{
	DECLARE_DYNAMIC(QueryDialog)

public:
	QueryDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~QueryDialog();

// Dialog Data
	enum { IDD = IDD_SQL_QUERY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString query;
};
