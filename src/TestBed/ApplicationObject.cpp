// ApplicationObject.cpp: implementation of the ApplicationObject class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ApplicationObject.h"
#include "TreeNode.h"
#include "Connection.h"
#include "Editor.h"
#include "ScriptDialog.h"
#include "Script.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ApplicationObject::ApplicationObject(const char *nam, ApplicationObject *prnt, const char *type, bool term)
{
	init();
	name = nam;
	typeName = type;
	terminal = term;

	if (parent = prnt)
		database = parent->database;
}

ApplicationObject::~ApplicationObject()
{
	if (editor)
		editor->DestroyWindow();

	FOR_OBJECTS (ApplicationObject*, child, &children)
		delete child;
	END_FOR;
}

ApplicationObject::ApplicationObject()
{
	init();
}


void ApplicationObject::init()
{
	populated = false;
	sorted = true;
	editor = NULL;
	extensible = false;
	deletable = false;
	editable = false;
	sqlable = true;
	canXref = false;
	options = 0;
	database = NULL;
	wasOpen = false;
}

bool ApplicationObject::expand()
{
	return false;
}

void ApplicationObject::populate()
{
	if (populated)
		return;

	refresh();
	populated = true;
}

void ApplicationObject::addChild(ApplicationObject * child)
{
	children.append (child);

	FOR_OBJECTS (TreeNode*, node, &instances)
		child->create (node);
	END_FOR;
}

TreeNode* ApplicationObject::create(TreeNode * nodeParent)
{
	TreeNode *node = new TreeNode (nodeParent, this, terminal);
	node->sorted = sorted;
	nodeParent->addChild (node);

	return node;
}

void ApplicationObject::createChildren(TreeNode * parentNode)
{
	FOR_OBJECTS (ApplicationObject*, child, &children)
		child->create (parentNode);
	END_FOR;
}

const char* ApplicationObject::getApplicationName()
{
	if (parent)
		return parent->getApplicationName();

	return "base";
}

bool ApplicationObject::doubleClick(TreeNode *node)
{
	return false;
}

bool ApplicationObject::editorClosed(CEditor * edit)
{
	if (edit->isChanged())
		{
		CString msg;
		msg.Format ("Save changes to %s?", (const char*) name);
		int ret = AfxMessageBox (msg, MB_YESNO);
		if (ret == IDYES)
			update (edit->getText());
		}

	editor = NULL;

	return true;
}

void ApplicationObject::openEditWindow()
{
	if (!editor)
		editor = createEditWindow();

	editor->ShowWindow (SW_SHOW);
	editor->BringWindowToTop();
}

CString ApplicationObject::getText()
{
	CString text;

	return text;
}

CString ApplicationObject::getName()
{
	return name;
}

void ApplicationObject::update()
{
	updateChildren();
}

void ApplicationObject::updateChildren()
{
	FOR_OBJECTS (ApplicationObject*, child, &children)
		child->update();
	END_FOR;
}


void ApplicationObject::deleteObject()
{
	FOR_OBJECTS (TreeNode*, node, &instances)
		node->deleteItem();
	END_FOR;

	parent->childDeleted (this);
}

void ApplicationObject::edit()
{

}

void ApplicationObject::extend()
{

}

CString ApplicationObject::getNameSpace()
{
	if (parent)
		return parent->getNameSpace();

	CString string;

	return string;

}

CEditor* ApplicationObject::createEditWindow()
{
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS( CEditor );
	CEditor *editor = (CEditor*) pRuntimeClass->CreateObject();
	editor->LoadFrame (IDR_EDITWINDOW, WS_OVERLAPPEDWINDOW, NULL );
	editor->setObject (this, getFullName(), getText());

	return editor;
}

void ApplicationObject::deleteChildren()
{
	FOR_OBJECTS (ApplicationObject*, child, &children)
		child->deleteObject();
	END_FOR;

	children.clear();
}

void ApplicationObject::addInstance(TreeNode * node)
{
	instances.append (node);
}

void ApplicationObject::instanceDeleted(TreeNode * node)
{
	instances.deleteItem (node);
}

void ApplicationObject::childDeleted(ApplicationObject * child)
{
	children.deleteItem (child);
}

bool ApplicationObject::changePending()
{
	return editor && editor->isChanged();
}

bool ApplicationObject::build()
{
	return true;
}

void ApplicationObject::update(const char * text)
{

}

/***
bool ApplicationObject::updateSql()
{
	if (!editor)
		return false;

	if (!database->execSql (editor->getText(), getNameSpace()))
		return false;

	editor->clearChange();

	return true;
}

CSqlWindow* ApplicationObject::createSqlWindow()
{
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS( CSqlWindow );
	CSqlWindow *editor = (CSqlWindow*) pRuntimeClass->CreateObject();
	editor->LoadFrame (IDR_EDITWINDOW, WS_OVERLAPPEDWINDOW, NULL );
	editor->setObject (this, getFullName(), getText());
	CString nameSpace = getNameSpace();
	editor->setDatabase (database, nameSpace);
	editor->ShowWindow (SW_SHOW);
	editor->BringWindowToTop();

	return editor;
}
***/

ApplicationObject* ApplicationObject::findChild(const char * name)
{
	FOR_OBJECTS (ApplicationObject*, child, &children)
		if (child->name == name)
			return child;
	END_FOR;

	return NULL;
}

void ApplicationObject::refresh()
{

}

void ApplicationObject::requestDelete()
{
	deleteObject();
}

void ApplicationObject::drop(ApplicationObject * dropObject)
{

}

bool ApplicationObject::dropEnable(ApplicationObject * dropObject)
{
	return false;
}

int ApplicationObject::findReferences()
{
	return 0;
}

int ApplicationObject::search(const char * text)
{
	int count = 0;

	FOR_OBJECTS (ApplicationObject*, child, &children)
		count += child->search (text);
	END_FOR;

	return count;
}

void ApplicationObject::editorDeleted(CEditor * window)
{
	editor = NULL;
}

CString ApplicationObject::getFullName()
{
	return getName();
}

void ApplicationObject::refreshAll()
{
	if (populated)
		refresh();

	FOR_OBJECTS (ApplicationObject*, child, &children)
		child->refreshAll();
	END_FOR;
}

void ApplicationObject::dropFile(const char * fileName)
{

}

void ApplicationObject::clearFlags()
{
	FOR_OBJECTS (ApplicationObject*, child, &children)
		child->alive = false;
	END_FOR;
}

void ApplicationObject::markAlive()
{
	alive = true;
}

void ApplicationObject::sweepChildren()
{
	FOR_OBJECTS (ApplicationObject*, child, &children)
		if (!child->alive)
			child->requestDelete();
	END_FOR;
}

long ApplicationObject::getOptions()
{
	return options;
}

bool ApplicationObject::doOption(long option)
{
	if (option & OPTION_ADD_SCRIPT)
		{
		CScriptDialog dialog;
		if (dialog.DoModal() == IDOK)
			{
			Script *script = new Script (dialog.m_name, this);
			addChild (script);
			script->openEditWindow();
			return true;
			}
		}

	return false;
}

bool ApplicationObject::isOpen()
{
	return !instances.isEmpty();
}

void ApplicationObject::markChanged()
{
	if (parent)
		parent->markChanged();
}

Script* ApplicationObject::findScript(ApplicationObject *object, CString scriptName)
{
	Script *hit;

	FOR_OBJECTS (ApplicationObject*, child, &children)
		if (child != object && (hit = child->findScript (this, scriptName)))
			return hit;
	END_FOR;

	if (!parent || parent == object)
		return NULL;

	return parent->findScript (this, scriptName);
}

void ApplicationObject::editorChanged()
{
	markChanged();
}

void ApplicationObject::onSave(CEditor *editor)
{
	update (editor->getText());
	editor->clearChange();
	save(this);
}

void ApplicationObject::save(ApplicationObject *object)
{
	if (parent)
		parent->save(object);
}
