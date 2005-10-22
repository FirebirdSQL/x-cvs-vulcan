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
	Database *database = ((CMsgMgrDoc*)m_pDocument)->database;
	Connection *connection = database->connection;
	AddMsgDialog dialog;
	dialog.populate(database);
		
	while (dialog.DoModal() == IDOK)
		{
		int msgNumber;
		int facCode;
		
		PStatement statement = connection->prepareStatement(
			"select fac_code, max_number from facilities where facility=?");
		statement->setString(1, dialog.facility);
		RSet resultSet = statement->executeQuery();
		
		if (resultSet->next())
			{
			facCode = resultSet->getInt(1);
			msgNumber = resultSet->getInt(2);
			}
		else
			{
			AfxMessageBox("Can't find facility code");
			continue;
			}
		
		statement = connection->prepareStatement(
			"insert into messages (symbol,number,fac_code,module,routine,text,explanation,trans_notes) "
			"  values(?,?,?,?,?,?,?,?}");
		int n = 0;
		statement->setString(n++, dialog.symbol);
		statement->setInt(n++, msgNumber);
		statement->setInt(n++, facCode);
		statement->setString(n++, dialog.module);
		statement->setString(n++, dialog.routine);
		statement->setString(n++, dialog.text);
		statement->setString(n++, dialog.explanation);
		statement->setString(n++, dialog.translationNotes);
		statement->executeUpdate();
		
		statement = connection->prepareStatement(
			"update facilities set max_number=? where fac_code=?");
		statement->setInt(1, msgNumber + 1);
		statement->setInt(2, facCode);
		statement->executeUpdate();
		
		connection->commit();

		return;
		}
}

void CMsgMgrView::OnFacilitiesListfacilities()
{
	Database *database = ((CMsgMgrDoc*)m_pDocument)->database;
	Connection *connection = database->connection;
	PStatement statement = connection->prepareStatement(
		"select facility,fac_code,max_number,last_change from facilities order by fac_code");
	RSet resultSet = statement->executeQuery();
	displayResults("Facilites", resultSet);
}

void CMsgMgrView::displayResults(CString label, ResultSet* resultSet)
{
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS( ResultWindow );
	ResultWindow *window = (ResultWindow*) pRuntimeClass->CreateObject();
	window->LoadFrame (IDR_EDITWINDOW, WS_OVERLAPPEDWINDOW, NULL );
	window->populate (label, resultSet);
	window->ShowWindow (SW_SHOW);
	window->BringWindowToTop();
}

void CMsgMgrView::OnMessagesSummary()
{
	try
		{
		Database *database = ((CMsgMgrDoc*)m_pDocument)->database;
		Connection *connection = database->connection;
		PStatement statement = connection->prepareStatement(
			"select facility, max(number), count(*), max(max_number) as last_assigned " 
			" from messages m, facilities f "
			" where m.fac_code=f.fac_code "
			" group by f.facility");
		RSet resultSet = statement->executeQuery();
		displayResults("Message Summary", resultSet);
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void CMsgMgrView::OnMessagesListmessages()
{
	Database *database = ((CMsgMgrDoc*)m_pDocument)->database;
	Connection *connection = database->connection;
	ListMsgsDialog dialog;
	dialog.populate(database);
	
	if (dialog.DoModal() == IDOK)
		{
		CString sql = "select number,symbol,text from messages m, facilities f "
					  "  where m.fac_code=f.fac_code and facility=? ";
		
		switch (dialog.order)
			{
			case number:
				sql += "order by number";
				break;
				
			case symbol:
				sql += "order by symbol";
				break;
				
			case text:
				sql += "order by text";
				break;
				
			}
		
		try
			{
			PStatement statement = connection->prepareStatement(sql);
			statement->setString(1, dialog.facility);
			RSet resultSet = statement->executeQuery();
			displayResults("Messages", resultSet);
			}
		catch (SQLException& exception)
			{
			AfxMessageBox(exception.getText());
			}
		}
}

void CMsgMgrView::OnFacilitiesNewfacility()
{
	Database *database = ((CMsgMgrDoc*)m_pDocument)->database;
	Connection *connection = database->connection;
	AddFacilityDialog dialog;
	PStatement statement = connection->prepareStatement(
		"select max(fac_code+1) from facilities");
	RSet resultSet = statement->executeQuery();
	resultSet->next();
	dialog.facCode = resultSet->getString(1);
	//resultSet->close();
	
	if (dialog.DoModal() == IDOK)
		{
		try
			{
			statement = connection->prepareStatement(
				"insert into facilities(facility,fac_code,max_number) values (?,?,1)");
			int n = 1;
			statement->setString(n++, dialog.facility);
			statement->setString(n++, dialog.facCode);
			statement->executeUpdate();
			connection->commit();
			}
		catch (SQLException& exception)
			{
			AfxMessageBox(exception.getText());
			}
		}
		
}
