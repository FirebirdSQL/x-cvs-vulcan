
// TreeNode.cpp: implementation of the TreeNode class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "stdafx.h"
#include "TestBed.h"
#include "TreeNode.h"
#include "ApplicationObject.h"
//#include "Serial.h"
//#include "SearchDialog.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TreeNode::TreeNode(TreeNode *parent, ApplicationObject *obj, bool terminal)
{
	init (parent, obj, terminal);
}

TreeNode::TreeNode(CTreeCtrl * control, ApplicationObject * obj)
{
	treeControl = control;
	init (NULL, obj, false);
}

void TreeNode::init(TreeNode *prnt, ApplicationObject *obj, bool term)
{
	if (parent = prnt)
		treeControl = parent->treeControl;

	object = obj;
	object->addInstance (this);
	name = object->name;
	bold = false;
	image = treeImageNone;
	sorted = true;
	empty = false;
	image = treeImageFolder;
	selectedImage = treeImageNone;
	atEnd = false;
	item = 0;
	terminal = term;
	expanded = false;
	extensible = object->extensible;
	deletable = object->deletable;
	editable = object->editable;
	sqlable = object->sqlable;
	canXref = object->canXref;
}

TreeNode::~TreeNode()
{
	FOR_OBJECTS (TreeNode*, child, &children)
		delete child;
	END_FOR;
}

void TreeNode::create()
{
	setAttributes();
}

void TreeNode::update()
{
	FOR_OBJECTS (TreeNode*, child, &children)
		child->update();
	END_FOR;
}


TreeNode* TreeNode::findNode(HTREEITEM itm)
{
	if (item == itm)
		return this;

	FOR_OBJECTS (TreeNode*, child, &children)
		TreeNode *hit = child->findNode (itm);
		if (hit)
			return hit;
	END_FOR;

	return NULL;
}

bool TreeNode::doubleClick()
{
	return object->doubleClick (this);
}

void TreeNode::expand(UINT state)
{
	if (state & TVIS_EXPANDED)
		{
		close();
		TV_ITEM item;
		item.hItem = this->item;
		item.mask = TVIF_STATE | TVIF_HANDLE;
		item.stateMask = TVIS_EXPANDED;
		item.state = 0;
		treeControl->SetItem (&item);
		return;
		}

	if (!expanded)
		{
		expanded = true;
		populate();
		FOR_OBJECTS (TreeNode*, child, &children)
			child->create();
		END_FOR;
		}

	if (!terminal && (empty != children.isEmpty()))
		{
		empty = !empty;
		setAttributes();
		}
}

void TreeNode::close()
{
	deleteChildren();
	select();
}

void TreeNode::deleteChildren()
{
	FOR_OBJECTS (TreeNode*, child, &children)
		child->deleteItem();
		//deleteChild (child);
	END_FOR;

	expanded = false;
	empty = false;
	setAttributes();
}

/***
void TreeNode::deleteChild(TreeNode * child)
{
	children.deleteItem (child);
	child->deleteItem();
	delete child;
}
***/

void TreeNode::deleteItem()
{
	object->instanceDeleted (this);
	deleteChildren();

	if (parent)
		parent->childDeleted (this);


	if (item)
		{
		treeControl->DeleteItem (item);
		item = NULL;
		//zapChildren();
		}

	delete this;
}

/***
void TreeNode::zapChildren()
{
	item = NULL;

	FOR_OBJECTS (TreeNode*, child, &children)
		child->zapChildren();
	END_FOR;
}
***/

void TreeNode::addChild(TreeNode * child)
{
	children.append (child);
}

void TreeNode::populate()
{
	object->populate ();

	if (children.isEmpty())
		object->createChildren (this);
}

void TreeNode::setAttributes()
{
	TV_INSERTSTRUCT insert;
	memset (&insert, 0, sizeof (insert));

	if (parent)
		insert.hParent = parent->item;

	insert.hInsertAfter = (!atEnd && parent && parent->sorted) ? TVI_SORT : TVI_LAST;
	insert.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_STATE | 
//					   TVIF_PARAM | TVIS_DROPHILITED | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					   TVIF_PARAM | TVIS_DROPHILITED;

	if (!terminal && !empty)
		insert.item.cChildren = 1;

	insert.item.state = TVIS_DROPHILITED;
	insert.item.lParam = (long) this;
	insert.item.pszText = (char*) (const char*) name;
	insert.item.iImage = image;
	insert.item.iSelectedImage = (selectedImage == treeImageNone) ? image : selectedImage;

	if (bold)
		insert.item.state |= TVIS_BOLD;

	insert.item.stateMask = TVIS_BOLD;

	if (item)
		{
		insert.item.hItem = item;
		insert.item.mask |= TVIF_HANDLE;
		treeControl->SetItem (&insert.item);
		}
	else
		item = treeControl->InsertItem (&insert);
}

void TreeNode::setName(const char * newName)
{
	name = newName;
	setAttributes();
}

void TreeNode::popupMenu(CWnd * window, CPoint & point)
{
	select();
	CMenu menu;
	UINT	menuResource = getMenu();

	if (!menu.LoadMenu (menuResource))
		return;

	CMenu *pMenu = menu.GetSubMenu (0);
	CPoint screen = point;
	window->ClientToScreen (&screen);
	CWnd*  pMainWindow = AfxGetMainWnd();
	int ret = pMenu->TrackPopupMenu (TPM_LEFTALIGN | TPM_RIGHTBUTTON,
						 screen.x, screen.y, pMainWindow); //window); //pMainWindow);
}

BOOL TreeNode::select()
{
	return treeControl->Select (item, TVGN_CARET);
}

UINT TreeNode::getMenu()
{
	return IDR_TREE_POPUP;
}

void TreeNode::deleteObject()
{
	CString msg;
	msg.Format ("Are you sure you want to delete %s %s?", 
				(const char*) object->typeName,
				(const char*) object->getFullName());

	if (AfxMessageBox (msg, MB_YESNO) == IDYES)
		object->requestDelete();
}

/***
void TreeNode::doSql()
{
	object->createSqlWindow();
}
***/

void TreeNode::edit()
{
	object->edit();
}

void TreeNode::extend()
{
	object->extend();
}

void TreeNode::childDeleted(TreeNode * child)
{
	children.deleteItem (child);
}

/***
void TreeNode::serialOpen(CString parentName, CArchive & ar)
{
	if (!expanded)
		return;

	CString path = parentName;

	if (parentName != "")
		path += ".";

	path += name;
	ar << saveOpenNode;
	ar << path;

	FOR_OBJECTS (TreeNode*, child, &children)
		child->serialOpen (path, ar);
	END_FOR;
}
***/

void TreeNode::openPath(const char * path)
{
	for (const char *p = path; *p && *p != '.'; ++p)
		;

	if (strncmp (name, path, p - path))
		return;

	if (!expanded)
		{
		expand (0);
		forceExpansion();
		}

	if (*p++)
		FOR_OBJECTS (TreeNode*, child, &children)
			child->openPath (p);
		END_FOR;
}

void TreeNode::forceExpansion()
{
	treeControl->Expand (item, TVE_EXPAND);
}

bool TreeNode::changePending()
{
	if (object->changePending())
		return true;

	FOR_OBJECTS (TreeNode*, child, &children)
		if (child->changePending())
			return true;
	END_FOR;

	return false;
}

void TreeNode::buildAll()
{
	if (!object->build())
		return;

	FOR_OBJECTS (TreeNode*, child, &children)
		child->buildAll();
	END_FOR;
}

bool TreeNode::dropEnable(TreeNode * dragObject)
{
	return object->dropEnable (dragObject->object);
}

void TreeNode::drop(TreeNode * dragObject)
{
	object->drop (dragObject->object);
}

void TreeNode::findReferences()
{
	if (object->findReferences() == 0)
		AfxMessageBox ("No references found");
}

/***
void TreeNode::search()
{
	CSearchDialog dialog;

	while (dialog.DoModal() == IDOK)
		{
		int count = object->search (dialog.m_string);
		if (count)
			break;
		AfxMessageBox ("No objects found");
		}
}
***/

void TreeNode::refreshAll()
{
	object->refreshAll();
}

void TreeNode::dropFile(const char * fileName)
{
	object->dropFile (fileName);
}

long TreeNode::getOptions()
{
	return object->getOptions();
}

bool TreeNode::doOption(long option)
{
	bool ret = object->doOption (option);

	if (ret)
		{
		expand (0);
		forceExpansion();
		}

	return ret;
}

void TreeNode::reopen()
{
	if (object->wasOpen)
		FOR_OBJECTS (ApplicationObject*, obj, &object->children)
			if (obj->wasOpen)
				{
				forceExpansion();
				FOR_OBJECTS (TreeNode*, child, &children)
					child->reopen();
				END_FOR;
				}
			break;
		END_FOR;
}
