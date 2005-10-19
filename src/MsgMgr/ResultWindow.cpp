// ResultWindow.cpp : implementation file
//

#include <memory.h>
#include "stdafx.h"
#include "MsgMgr.h"
#include "ResultWindow.h"
#include "Connection.h"
#include "Types.h"
#include "Stream.h"

#define SELECTION_COLOR		RGB (0, 0, 128)
#define WHITE				RGB (255, 255, 255)
#define BLACK				RGB (0, 0, 0)
#define MAX(a,b)			((a > b) ? (a) : (b))
#define MIN(a,b)			((a < b) ? (a) : (b))
#define PRINT_VERT_MARGIN	6
#define PRINT_HORZ_MARGIN	6

enum Align {
	Center,
	Left,
	Right,
	};

struct Column {
    int			headerWidth;
	int			dataWidth;
	int			offset;
	int			type;
	Align		alignment;
	const char	*segment;
	};


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// ResultWindow

IMPLEMENT_DYNCREATE(ResultWindow, CMDIChildWnd)

ResultWindow::ResultWindow()
{
	//textLength = 0;
}

ResultWindow::~ResultWindow()
{
	FOR_OBJECTS (CString*, row, &rows)
		delete [] row;
	END_FOR;
}


BEGIN_MESSAGE_MAP(ResultWindow, CMDIChildWnd)
	//{{AFX_MSG_MAP(ResultWindow)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ResultWindow message handlers

void ResultWindow::populate(const char *windowName, ResultSet * resultSet)
{
	name = windowName;
	SetWindowText (name);
	ResultSetMetaData *metaData = resultSet->getMetaData();
	numberColumns = metaData->getColumnCount();
	Column *columns = new Column [numberColumns];
	memset (columns, 0, sizeof (Column) * numberColumns);
	//totalLines = 2;
	Stream stream (1024);

	for (int n = 0; n < numberColumns; ++n)
		{
		Column *column = columns + n;
		const char *string = metaData->getColumnName (n + 1);
		CSize size = getStringSize (string);
		column->headerWidth = size.cx;
		column->type = metaData->getColumnType (n + 1);
		switch (column->type)
			{
			case TINYINT:
			case SMALLINT:
			case INTEGER:
			case BIGINT:
			case jdbcFLOAT:
			case jdbcDOUBLE:
				column->alignment = Right;
				break;

			default:
				column->alignment = Left;
			}
		}

	while (resultSet->next())
		{
		CString *row = new CString [numberColumns];
		rows.append (row);
		for (int n = 0; n < numberColumns; ++n)
			{
			Column *column = columns + n;
			row [n] = resultSet->getString (n + 1);
			CSize size = getStringSize (row [n]);
			column->dataWidth = MAX (column->dataWidth, size.cx);
			}
		}

	int offset = 0;

	for (n = 0; n < numberColumns; ++n)
		{
		Column *column = columns + n;
		column->offset = offset;
		offset += MAX (column->dataWidth, column->headerWidth) + 1;
		}

	int position = 0;

	for (n = 0; n < numberColumns; ++n)
		{
		Column *column = columns + n;
		column->segment = metaData->getColumnName (n + 1);
		putSegment (column, Center, &stream, &position);
		}

	stream.putSegment ("\r\n");
	position = 0;

	for (n = 0; n < numberColumns; ++n)
		{
		Column *column = columns + n;
		int length = MAX (column->headerWidth, column->dataWidth);
		fill (column->offset - position, ' ', &stream);
		fill (length, '-', &stream);
		position = column->offset + length;
		}

	stream.putSegment ("\r\n\r\n");
	position = 0;

	FOR_OBJECTS (CString*, row, &rows)
		bool first = true;
		for (bool again = true; again;)
			{
			again = false;
			for (n = 0; n < numberColumns; ++n)
				{
				Column *column = columns + n;
				if (first)
					column->segment = (char*)(const char*) row [n];
				if (column->segment && putSegment (column, column->alignment, &stream, &position))
					again = true;
				}
			first = false;
			stream.putSegment ("\r\n");
			position = 0;
			}
	END_FOR;

	char *data = stream.getString();
	edit.SetWindowText (data);
	totalLines = 1;

	for (const char *p = data; *p;)
		if (*p++ == '\n')
			++totalLines;

	delete [] columns;
	delete [] data;
}

BOOL ResultWindow::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CRect rect;
	GetClientRect (&rect);
	DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
				  ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE;
	edit.Create (style, rect, this, IDC_EDIT);
	font.CreatePointFont (100, "Courier");
	edit.SetFont (&font);

	LOGFONT logFont;
	memset (&logFont, 0, sizeof (logFont));
	logFont.lfHeight = 110;
	logFont.lfWeight = FW_BOLD;
	strcpy (logFont.lfFaceName, "Courier");
	headerFont.CreatePointFontIndirect (&logFont);
			
	return CMDIChildWnd::OnCreateClient(lpcs, pContext);
}

void ResultWindow::OnEditCopy() 
{
	edit.Copy();
}

void ResultWindow::OnSize(UINT nType, int cx, int cy) 
{
	CMDIChildWnd::OnSize(nType, cx, cy);
	
	CRect rect;
	GetClientRect (&rect);
	edit.MoveWindow (rect);
}


CSize ResultWindow::getStringSize(const char * string)
{
	const char *start = string;
	const char *nonWhite = string;
	int max = 0;
	int l;
	int lines = 1;

	for (const char *p = string; *p; ++p)
		switch (*p)
			{
			case '\n':
				l = nonWhite + 1 - start;
				max = MAX (max, nonWhite + 1 - start);
				start = nonWhite = p + 1;
				if (p [1])
					++lines;
				break;

			case '\r':
			case ' ':
			case '\t':
				break;

			default:
				nonWhite = p;
			}

	max = MAX (max, nonWhite + 1 - start);
	CSize size (max, lines);
	
	return size;	
}

bool ResultWindow::putSegment(Column *column, Align alignment, Stream *stream, int *positionPtr)
{
	int start = *positionPtr;
	int position = column->offset;
	const char *text = column->segment;

	if (column->headerWidth > column->dataWidth)
		position += (column->headerWidth - column->dataWidth) / 2;


	for (const char *p = text; *p; ++p)
		if (*p == '\n' || *p == '\r')
			break;

	column->segment = (*p) ? p + 1 : NULL;

	while (p > text && p [-1] == ' ' || p [-1] == '\t')
		--p;

	int l = p - text;

	switch (alignment)
		{
		case Center:
			position += (column->dataWidth - l) / 2;
			break;

		case Left:
			break;

		case Right:
			position += column->dataWidth - l;
			break;
		}

	fill (position - start, ' ', stream);
	stream->putSegment (l, text, true);
	position += l;

	while (*p && (*p == ' ' || *p == '\r' || *p == '\n'))
		++p;

	//column->segment = (*p) ? p : NULL;
	*positionPtr = position;

	return column->segment != NULL;
}

void ResultWindow::OnFilePrint() 
{
	CPrintInfo printInfo;
	CPrintInfo *pInfo = &printInfo;
	CWinApp* pApp = AfxGetApp();
	pInfo->m_pPD->m_pd.nFromPage = (WORD)pInfo->GetMinPage();
	pInfo->m_pPD->m_pd.nToPage = (WORD)pInfo->GetMaxPage();

	if (pApp->DoPrintDialog(pInfo->m_pPD) != IDOK)
		return;       // do not print

	CString strOutput;

	if (printInfo.m_pPD->m_pd.Flags & PD_PRINTTOFILE)
		{
		// construct CFileDialog for browsing
		CString strDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULTEXT));
		CString strPrintDef(MAKEINTRESOURCE(AFX_IDS_PRINTDEFAULT));
		CString strFilter(MAKEINTRESOURCE(AFX_IDS_PRINTFILTER));
		CString strCaption(MAKEINTRESOURCE(AFX_IDS_PRINTCAPTION));
		CFileDialog dlg(FALSE, strDef, strPrintDef,
			OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, strFilter);
		dlg.m_ofn.lpstrTitle = strCaption;

		if (dlg.DoModal() != IDOK)
			return;

		// set output device to resulting path name
		strOutput = dlg.GetPathName();
		}

	DOCINFO docInfo;
	memset(&docInfo, 0, sizeof(DOCINFO));
	docInfo.cbSize = sizeof(DOCINFO);
	CString strTitle;
	GetWindowText (strTitle);
	docInfo.lpszDocName = strTitle;
	CString strPortName;
	int nFormatID;

	if (strOutput.IsEmpty())
		{
		docInfo.lpszOutput = NULL;
		strPortName = printInfo.m_pPD->GetPortName();
		nFormatID = AFX_IDS_PRINTONPORT;
		}
	/***
	else
		{
		docInfo.lpszOutput = strOutput;
		AfxGetFileTitle(strOutput,
			strPortName.GetBuffer(_MAX_PATH), _MAX_PATH);
		nFormatID = AFX_IDS_PRINTTOFILE;
		}
	***/

	// setup the printing DC
	CDC dcPrint;
	dcPrint.Attach(printInfo.m_pPD->m_pd.hDC);  // attach printer dc
	dcPrint.m_bPrinting = TRUE;
	OnBeginPrinting(&dcPrint, &printInfo);
	//dcPrint.SetAbortProc(_AfxAbortProc);

	// disable main window while printing & init printing status dialog
	AfxGetMainWnd()->EnableWindow(FALSE);
	CString strTemp;

	/***
	CPrintingDialog dlgPrintStatus(this);
	dlgPrintStatus.Create (CPrintingDialog::IDD, this);

	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_DOCNAME, strTitle);
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PRINTERNAME,
		printInfo.m_pPD->GetDeviceName());
	AfxFormatString1(strTemp, nFormatID, strPortName);
	dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PORTNAME, strTemp);
	dlgPrintStatus.ShowWindow(SW_SHOW);
	dlgPrintStatus.UpdateWindow();
	***/

	// Figure out maximum number of lines

	printInfo.m_rectDraw.SetRect(0, 0,
			dcPrint.GetDeviceCaps(HORZRES),
			dcPrint.GetDeviceCaps(VERTRES));
	dcPrint.DPtoLP(&printInfo.m_rectDraw);
	int pageLines = getLinesPerPage (&dcPrint, &printInfo);

	printInfo.SetMaxPage (totalLines / pageLines + 1);

	// start document printing process
	if (dcPrint.StartDoc(&docInfo) == SP_ERROR)
		{
		// enable main window before proceeding
		AfxGetMainWnd()->EnableWindow(TRUE);

		// cleanup and show error message
		OnEndPrinting(&dcPrint, &printInfo);
		//dlgPrintStatus.DestroyWindow();
		//dcPrint.Detach();   // will be cleaned up by CPrintInfo destructor
		AfxMessageBox(AFX_IDP_FAILED_TO_START_PRINT);
		return;
		}

	// Guarantee values are in the valid range
	UINT nEndPage = printInfo.GetToPage();
	UINT nStartPage = printInfo.GetFromPage();

	if (nEndPage < printInfo.GetMinPage())
		nEndPage = printInfo.GetMinPage();
	if (nEndPage > printInfo.GetMaxPage())
		nEndPage = printInfo.GetMaxPage();

	if (nStartPage < printInfo.GetMinPage())
		nStartPage = printInfo.GetMinPage();
	if (nStartPage > printInfo.GetMaxPage())
		nStartPage = printInfo.GetMaxPage();

	int nStep = (nEndPage >= nStartPage) ? 1 : -1;
	nEndPage = (nEndPage == 0xffff) ? 0xffff : nEndPage + nStep;

	VERIFY(strTemp.LoadString(AFX_IDS_PRINTPAGENUM));

	// begin page printing loop
	BOOL bError = FALSE;

	for (printInfo.m_nCurPage = nStartPage;
		 printInfo.m_nCurPage != nEndPage; printInfo.m_nCurPage += nStep)
		{
		OnPrepareDC(&dcPrint, &printInfo);

		// check for end of print
		if (!printInfo.m_bContinuePrinting)
			break;

		// write current page
		TCHAR szBuf[80];
		wsprintf(szBuf, strTemp, printInfo.m_nCurPage);
		//dlgPrintStatus.SetDlgItemText(AFX_IDC_PRINT_PAGENUM, szBuf);

		// set up drawing rect to entire page (in logical coordinates)
		printInfo.m_rectDraw.SetRect(0, 0,
			dcPrint.GetDeviceCaps(HORZRES),
			dcPrint.GetDeviceCaps(VERTRES));
		dcPrint.DPtoLP(&printInfo.m_rectDraw);

		// attempt to start the current page
		if (dcPrint.StartPage() < 0)
			{
			bError = TRUE;
			break;
			}

		// must call OnPrepareDC on newer versions of Windows because
		// StartPage now resets the device attributes.
		/***
		if (afxData.bMarked4)
			OnPrepareDC(&dcPrint, &printInfo);
		***/

		ASSERT(printInfo.m_bContinuePrinting);

		// page successfully started, so now render the page
		OnPrint(&dcPrint, &printInfo);

		if (dcPrint.EndPage() < 0) // || !_AfxAbortProc(dcPrint.m_hDC, 0))
			{
			bError = TRUE;
			break;
			}
		}

	// cleanup document printing process
	if (!bError)
		dcPrint.EndDoc();
	else
		dcPrint.AbortDoc();

	AfxGetMainWnd()->EnableWindow();    // enable main window

	//OnEndPrinting(&dcPrint, &printInfo);    // clean up after printing
	//dlgPrintStatus.DestroyWindow();

	dcPrint.Detach();   // will be cleaned up by CPrintInfo destructor
}

void ResultWindow::OnBeginPrinting(CDC *pDC, CPrintInfo *printInfo)
{
	printInfo->SetMinPage (1);
	printInfo->SetMaxPage (1);

	CRect windowSize;
	GetClientRect (&windowSize);
	CRect pageSize (0, 0, pDC->GetDeviceCaps(HORZRES), 
						  pDC->GetDeviceCaps(VERTRES));

	pDC->SetMapMode (MM_ISOTROPIC);
	pDC->SetWindowExt (windowSize.Width(), windowSize.Height());
	pDC->SetViewportExt (pageSize.Width(), pageSize.Height());

	CSize s1 = pDC->GetWindowExt();
	CSize s2 = pDC->GetViewportExt();
	pDC->SetViewportOrg (0, (pageSize.bottom - s2.cy) / 5);
}

void ResultWindow::OnEndPrinting(CDC *pDC, CPrintInfo *printInfo)
{

}

void ResultWindow::OnPrepareDC(CDC *pDC, CPrintInfo *printInfo)
{

}

void ResultWindow::OnPrint(CDC *pDC, CPrintInfo *printInfo)
{
	CSize size = pDC->GetTextExtent ("M", 1);
	int fontWidth = size.cx;
	int fontHeight = size.cy;
	int pageNumber = printInfo->m_nCurPage;
	int pageLines = getLinesPerPage (pDC, printInfo);
	int margin = fontWidth * PRINT_HORZ_MARGIN;
	int topLineNumber, bottomLineNumber;
	int tabStops = 4;
	int tabWidth = fontWidth * tabStops;

	void *oldFont = pDC->SelectObject (headerFont);
	size = pDC->GetTextExtent ("M", 1);
	int x = fontWidth * PRINT_HORZ_MARGIN / 2;
	int y = fontHeight * PRINT_VERT_MARGIN / 2  + printInfo->m_rectDraw.top;
	pDC->TextOut (x, y, name, name.GetLength());
	CPen pen (PS_SOLID, 1, BLACK);
	void *oldPen = pDC->SelectObject (&pen);
	pDC->MoveTo (x, y + size.cy);
	pDC->LineTo (printInfo->m_rectDraw.right - x, y + size.cy);
	pDC->SelectObject (oldPen);
	pDC->SelectObject (oldFont);

	topLineNumber = pageLines * (pageNumber - 1);
	bottomLineNumber = MIN (topLineNumber + pageLines, totalLines);

	CString text;
	edit.GetWindowText (text);
	int line = 0;
	
	for (const char *p = text; *p; ++line)
		{
		const char *start = p;
		while (*p && (*p++ != '\n'))
			;
		int length = p - start - 1;
		if (line >= topLineNumber && line < bottomLineNumber && length > 0)
			{
			if (start [length - 1] == '\r')
				--length;
			int xLoc = margin;
			int yLoc = (PRINT_VERT_MARGIN + line - topLineNumber) * fontHeight + printInfo->m_rectDraw.top;
			pDC->TabbedTextOut (xLoc, yLoc, start, length, 1, &tabWidth, margin);
			}
		}

}

int ResultWindow::getLinesPerPage(CDC *pDC, CPrintInfo *printInfo)
{
	CSize size = pDC->GetTextExtent ("M", 1);
	int lines = printInfo->m_rectDraw.Height() / size.cy;

	return lines - PRINT_VERT_MARGIN;
}

void ResultWindow::fill(int count, char character, Stream *stream)
{
	for (int n = 0; n < count; ++n)
		stream->putCharacter (character);
}
