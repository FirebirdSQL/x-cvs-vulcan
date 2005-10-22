// MsgMgrDoc.cpp : implementation of the CMsgMgrDoc class
//

#include "stdafx.h"
#include "MsgMgr.h"

#include "MsgMgrDoc.h"
#include "Database.h"
#include ".\msgmgrdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMsgMgrDoc

IMPLEMENT_DYNCREATE(CMsgMgrDoc, CDocument)

BEGIN_MESSAGE_MAP(CMsgMgrDoc, CDocument)
END_MESSAGE_MAP()


// CMsgMgrDoc construction/destruction

CMsgMgrDoc::CMsgMgrDoc()
{
	// TODO: add one-time construction code here
	database = NULL;
}

CMsgMgrDoc::~CMsgMgrDoc()
{
	delete database;
}

BOOL CMsgMgrDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CMsgMgrDoc serialization

void CMsgMgrDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CMsgMgrDoc diagnostics

#ifdef _DEBUG
void CMsgMgrDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMsgMgrDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMsgMgrDoc commands

void CMsgMgrDoc::setDatabase(Database *db)
{
	database = db;
	SetTitle(db->name);
}

