// TestBedView.h : interface of the CTestBedView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TESTBEDVIEW_H__B96C96CC_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_TESTBEDVIEW_H__B96C96CC_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class TreeNode;


class CTestBedView : public CTreeView
{
protected: // create from serialization only
	CTestBedView();
	DECLARE_DYNCREATE(CTestBedView)

// Attributes
public:
	CTestBedDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestBedView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	void markChanged();
	virtual TreeNode* getSelectedNode();
	TreeNode* tree;
	virtual ~CTestBedView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTestBedView)
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPopupNewfolder();
	afx_msg void OnUpdatePopupNewfolder(CCmdUI* pCmdUI);
	afx_msg void OnPopupNewscript();
	afx_msg void OnUpdatePopupNewscript(CCmdUI* pCmdUI);
	afx_msg void OnEditDelete();
	afx_msg void OnUpdateEditDelete(CCmdUI* pCmdUI);
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in TestBedView.cpp
inline CTestBedDoc* CTestBedView::GetDocument()
   { return (CTestBedDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTBEDVIEW_H__B96C96CC_0584_11D4_98DB_0000C01D2301__INCLUDED_)
