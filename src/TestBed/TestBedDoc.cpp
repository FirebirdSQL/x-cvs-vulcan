// TestBedDoc.cpp : implementation of the CTestBedDoc class
//

#include "stdafx.h"
#include "TestBed.h"

#include "TestBedDoc.h"
#include "TestEnv.h"
#include "Storage.h"
#include "OcsArchive.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestBedDoc

IMPLEMENT_DYNCREATE(CTestBedDoc, CDocument)

BEGIN_MESSAGE_MAP(CTestBedDoc, CDocument)
	//{{AFX_MSG_MAP(CTestBedDoc)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestBedDoc construction/destruction

CTestBedDoc::CTestBedDoc()
{
	topNode = new TestEnv (this);
}

CTestBedDoc::~CTestBedDoc()
{
	if (topNode)
		delete topNode;
}

BOOL CTestBedDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CTestBedDoc serialization

void CTestBedDoc::Serialize(CArchive& ar)
{
	COcsArchive archive (ar);

	if (ar.IsStoring())
		{
		OCS_WRITE (storage, ((OCS_ENV) archive), ApplicationObject, topNode, &ar);
		}
	else
		{
		int	type;
		if (topNode)
			delete topNode;
		topNode = NULL;
		OCS_READ (storage, ((OCS_ENV) archive), &type, &topNode, &ar);
		topNode->setDocument (this);
		}
}

/////////////////////////////////////////////////////////////////////////////
// CTestBedDoc diagnostics

#ifdef _DEBUG
void CTestBedDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CTestBedDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTestBedDoc commands

void CTestBedDoc::markChanged()
{
	SetModifiedFlag (true);
}

BOOL CTestBedDoc::SaveModified() 
{
	topNode->update();
	
	return CDocument::SaveModified();
}

void CTestBedDoc::OnFileSave() 
{
	OnSaveDocument(m_strPathName);	
}

void CTestBedDoc::OnUpdateFileSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (IsModified());
	
}

void CTestBedDoc::save()
{
	OnSaveDocument(m_strPathName);	
}
