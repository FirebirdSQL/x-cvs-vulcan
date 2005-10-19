// MsgMgrView.h : interface of the CMsgMgrView class
//


#pragma once


class CMsgMgrView : public CView
{
protected: // create from serialization only
	CMsgMgrView();
	DECLARE_DYNCREATE(CMsgMgrView)

// Attributes
public:
	CMsgMgrDoc* GetDocument() const;

// Operations
public:

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CMsgMgrView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNewMessage();
};

#ifndef _DEBUG  // debug version in MsgMgrView.cpp
inline CMsgMgrDoc* CMsgMgrView::GetDocument() const
   { return reinterpret_cast<CMsgMgrDoc*>(m_pDocument); }
#endif

