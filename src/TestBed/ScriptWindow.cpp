// ScriptWindow.cpp: implementation of the CScriptWindow class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ScriptWindow.h"
#include "Script.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CScriptWindow, CEditor)

CScriptWindow::CScriptWindow()
{

}

CScriptWindow::~CScriptWindow()
{

}

BEGIN_MESSAGE_MAP(CScriptWindow, CEditor)
	//{{AFX_MSG_MAP(CScriptWindow)
	ON_COMMAND(ID_EXECUTE, OnExecute)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CScriptWindow::OnExecute()
{
	if (client)
		((Script*) client)->execute();
}
