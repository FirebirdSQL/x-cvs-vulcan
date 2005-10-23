// MsgMgrView.cpp : implementation of the CMsgMgrView class
//

#include "stdafx.h"
#include "MsgMgr.h"

#include "MsgMgrDoc.h"
#include "MsgMgrView.h"
#include "AddMsgDialog.h"
#include "PStatement.h"
#include "RSet.h"
#include "Database.h"
#include "ResultWindow.h"
#include "ListMsgsDialog.h"
#include "AddFacilityDialog.h"
#include "UpdateMsgDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMsgMgrView

IMPLEMENT_DYNCREATE(CMsgMgrView, CView)

BEGIN_MESSAGE_MAP(CMsgMgrView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_COMMAND(ID_NEW_MESSAGE, OnNewMessage)
	ON_COMMAND(ID_FACILITIES_LISTFACILITIES, OnFacilitiesListfacilities)
	ON_COMMAND(ID_MESSAGES_SUMMARY, OnMessagesSummary)
	ON_COMMAND(ID_MESSAGES_LISTMESSAGES, OnMessagesListmessages)
	ON_COMMAND(ID_FACILITIES_NEWFACILITY, OnFacilitiesNewfacility)
	ON_COMMAND(ID_MESSAGES_UPDATEMESSAGE, OnMessagesUpdatemessage)
END_MESSAGE_MAP()

// CMsgMgrView construction/destruction

CMsgMgrView::CMsgMgrView()
{
	// TODO: add construction code here

}

CMsgMgrView::~CMsgMgrView()
{
}

BOOL CMsgMgrView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CMsgMgrView drawing

void CMsgMgrView::OnDraw(CDC* /*pDC*/)
{
	CMsgMgrDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CMsgMgrView printing

BOOL CMsgMgrView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CMsgMgrView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CMsgMgrView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}


// CMsgMgrView diagnostics

#ifdef _DEBUG
void CMsgMgrView::AssertValid() const
{
	CView::AssertValid();
}

void CMsgMgrView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMsgMgrDoc* CMsgMgrView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMsgMgrDoc)));
	return (CMsgMgrDoc*)m_pDocument;
}
#endif //_DEBUG


// CMsgMgrView message handlers

void CMsgMgrView::OnNewMessage()
{
	GetDocument()->database->addMessage();
}

void CMsgMgrView::OnFacilitiesListfacilities()
{
	GetDocument()->database->listFacilities();
}

void CMsgMgrView::OnMessagesSummary()
{
	GetDocument()->database->genSummary();
}

void CMsgMgrView::OnMessagesListmessages()
{
	GetDocument()->database->listMessages();
}

void CMsgMgrView::OnFacilitiesNewfacility()
{
	GetDocument()->database->addFacility();
}

void CMsgMgrView::OnMessagesUpdatemessage()
{
	GetDocument()->database->updateMessage();
}
