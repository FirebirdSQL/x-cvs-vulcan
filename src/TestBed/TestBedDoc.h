// TestBedDoc.h : interface of the CTestBedDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TESTBEDDOC_H__B96C96CA_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_TESTBEDDOC_H__B96C96CA_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class ApplicationObject;
class TestEnv;

class CTestBedDoc : public CDocument
{
protected: // create from serialization only
	CTestBedDoc();
	DECLARE_DYNCREATE(CTestBedDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestBedDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	protected:
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL

// Implementation
public:
	void save();
	void markChanged();
	TestEnv* topNode;
	virtual ~CTestBedDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTestBedDoc)
	afx_msg void OnFileSave();
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTBEDDOC_H__B96C96CA_0584_11D4_98DB_0000C01D2301__INCLUDED_)
