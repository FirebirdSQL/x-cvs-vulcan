// TestBedView.cpp : implementation of the CTestBedView class
//

#include "stdafx.h"
#include "TestBed.h"

#include "TestBedDoc.h"
#include "TestBedView.h"
#include "TreeNode.h"
#include "TestEnv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTestBedView

IMPLEMENT_DYNCREATE(CTestBedView, CTreeView)

BEGIN_MESSAGE_MAP(CTestBedView, CTreeView)
	//{{AFX_MSG_MAP(CTestBedView)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_POPUP_NEWFOLDER, OnPopupNewfolder)
	ON_UPDATE_COMMAND_UI(ID_POPUP_NEWFOLDER, OnUpdatePopupNewfolder)
	ON_COMMAND(ID_POPUP_NEWSCRIPT, OnPopupNewscript)
	ON_UPDATE_COMMAND_UI(ID_POPUP_NEWSCRIPT, OnUpdatePopupNewscript)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTestBedView construction/destruction

CTestBedView::CTestBedView()
{
	tree = NULL;
}

CTestBedView::~CTestBedView()
{
	if (tree)
		delete tree;
}

BOOL CTestBedView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CTestBedView drawing

void CTestBedView::OnDraw(CDC* pDC)
{
	CTestBedDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

void CTestBedView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();
	CTestBedDoc* pDoc = GetDocument();

	if (!tree)
		{
		tree = new TreeNode ((CTreeCtrl*) this, pDoc->topNode);
		tree->create();
		tree->reopen();
		}
}

/////////////////////////////////////////////////////////////////////////////
// CTestBedView printing

BOOL CTestBedView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTestBedView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTestBedView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CTestBedView diagnostics

#ifdef _DEBUG
void CTestBedView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CTestBedView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CTestBedDoc* CTestBedView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTestBedDoc)));
	return (CTestBedDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTestBedView message handlers

BOOL CTestBedView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	dwStyle |= WS_CHILD | WS_VISIBLE |
			  TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT |
			  TVS_SHOWSELALWAYS;
	
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void CTestBedView::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	TV_ITEM *item = &pNMTreeView->itemNew;
	CWaitCursor dummy;
	TreeNode *node = (TreeNode*) item->lParam;
	node->expand (item->state);
	SetTimer (0, 1000, NULL);
	*pResult = 0;
}

void CTestBedView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	if (tree)
		{
		UINT flags;
		HTREEITEM item = GetTreeCtrl().HitTest (point, &flags);
		TreeNode *node = tree->findNode (item);
		if (node &&	node->doubleClick ())
			return;
		}
	
	CTreeView::OnLButtonDblClk(nFlags, point);
}

void CTestBedView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	//CTreeView::OnRButtonDown(nFlags, point);
}

void CTestBedView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if (tree)
		{
		UINT flags;
		HTREEITEM item = GetTreeCtrl().HitTest (point, &flags);
		TreeNode *node = tree->findNode (item);
		if (node)
			node->popupMenu (this, point);
		}
	
	
	CTreeView::OnRButtonUp(nFlags, point);
}

TreeNode* CTestBedView::getSelectedNode()
{
	HTREEITEM item = GetTreeCtrl().GetSelectedItem();

	if (!item)
		return NULL;

	return tree->findNode (item);
}

void CTestBedView::OnPopupNewfolder() 
{
	TreeNode *node = getSelectedNode();

	if (node)
		if (node->doOption (OPTION_ADD_FOLDER))
			markChanged();
}

void CTestBedView::OnUpdatePopupNewfolder(CCmdUI* pCmdUI) 
{
	TreeNode *node = getSelectedNode();
	pCmdUI->Enable (node && (node->getOptions() & OPTION_ADD_FOLDER));
}

void CTestBedView::OnPopupNewscript() 
{
	TreeNode *node = getSelectedNode();

	if (node)
		if (node->doOption (OPTION_ADD_SCRIPT))
			markChanged();
}

void CTestBedView::OnUpdatePopupNewscript(CCmdUI* pCmdUI) 
{
	TreeNode *node = getSelectedNode();
	pCmdUI->Enable (node && (node->getOptions() & OPTION_ADD_SCRIPT));
}

void CTestBedView::markChanged()
{
	GetDocument()->markChanged();
}

void CTestBedView::OnEditDelete() 
{
	TreeNode *node = getSelectedNode();

	if (node)
		node->deleteObject();
}

void CTestBedView::OnUpdateEditDelete(CCmdUI* pCmdUI) 
{
	TreeNode *node = getSelectedNode();
	pCmdUI->Enable (node && node->deletable);
}
