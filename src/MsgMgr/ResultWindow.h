#if !defined(AFX_RESULTWINDOW_H__EDC02B96_5423_11D3_AB78_0000C01D2301__INCLUDED_)
#define AFX_RESULTWINDOW_H__EDC02B96_5423_11D3_AB78_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ResultWindow.h : header file
//

#include "LinkedList.h"

struct Column;
enum Align;

class ResultSet;
class Stream;

/////////////////////////////////////////////////////////////////////////////
// ResultWindow frame

class ResultWindow : public CMDIChildWnd
{
	DECLARE_DYNCREATE(ResultWindow)
protected:
	ResultWindow();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void fill (int count, char character, Stream *stream);
	int getLinesPerPage(CDC * pDC, CPrintInfo * printInfo);
	void OnPrint(CDC * pDC, CPrintInfo * printInfo);
	void OnPrepareDC(CDC * pDC, CPrintInfo * printInfo);
	void OnEndPrinting(CDC * pDC, CPrintInfo * printInfo);
	void OnBeginPrinting(CDC * pDC, CPrintInfo * printInfo);
	bool putSegment (Column *format, Align alignment, Stream *stream, int *positionPtr);
	CSize getStringSize (const char *string);
	void populate (const char *name, ResultSet *resultSet);

	CEdit		edit;
	LinkedList	rows;
	int			numberColumns;
	//int			textLength;
	int			totalLines;
	CFont		font;
	CFont		headerFont;
	CString		name;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(ResultWindow)
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~ResultWindow();

	// Generated message map functions
	//{{AFX_MSG(ResultWindow)
	afx_msg void OnEditCopy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnFilePrint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTWINDOW_H__EDC02B96_5423_11D3_AB78_0000C01D2301__INCLUDED_)
