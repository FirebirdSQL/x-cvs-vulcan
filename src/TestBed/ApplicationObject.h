// ApplicationObject.h: interface for the ApplicationObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APPLICATIONOBJECT_H__E5A0712D_4371_11D3_AB78_0000C01D2301__INCLUDED_)
#define AFX_APPLICATIONOBJECT_H__E5A0712D_4371_11D3_AB78_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "LinkedList.h"
#include "EditClient.h"

#define OPTION_ADD_FOLDER		1
#define OPTION_ADD_SCRIPT		2

class Database;
class TreeNode;
class CEditor;
class CSqlWindow;
class Script;

class ApplicationObject : public CEditClient
{
public:
	virtual void save (ApplicationObject *object);
	virtual void onSave (CEditor * editor);
	virtual void editorChanged();
	virtual Script* findScript (ApplicationObject *object, CString scriptName);
	virtual void markChanged();
	virtual bool isOpen();
	void init();
	 ApplicationObject();
	virtual bool doOption (long option);
	virtual long getOptions();
	void sweepChildren();
	void markAlive();
	virtual void clearFlags();
	virtual void dropFile (const char *fileName);
	virtual void refreshAll();
	virtual CString getFullName();
	void editorDeleted (CEditor *editor);
	//bool execSql (const char *sql);
	virtual int search (const char *text);
	virtual int findReferences();
	virtual bool dropEnable (ApplicationObject *dropObject);
	virtual void drop (ApplicationObject *dropObject);
	virtual void requestDelete();
	virtual void refresh();
	virtual ApplicationObject* findChild (const char *name);
	//virtual CSqlWindow* createSqlWindow();
	//virtual bool updateSql();
	virtual void update (const char *text);
	virtual bool build();
	virtual bool changePending();
	virtual void childDeleted (ApplicationObject *child);
	virtual void instanceDeleted (TreeNode *node);
	virtual void addInstance (TreeNode *node);
	virtual void deleteChildren();
	virtual CEditor* createEditWindow();
	virtual CString getNameSpace();
	virtual void extend();
	//virtual void doSql();
	virtual void edit();
	virtual void deleteObject();
	virtual void updateChildren();
	virtual void update();
	virtual CString getName();
	virtual CString getText();
	virtual void openEditWindow();
	virtual bool editorClosed (CEditor *editor);
	virtual bool doubleClick(TreeNode *node);
	virtual const char* getApplicationName();
	virtual void createChildren (TreeNode *parentNode);
	virtual TreeNode* create (TreeNode *nodeParent);
	virtual void addChild (ApplicationObject *child);
	virtual void populate ();
	virtual bool expand();
	ApplicationObject(const char *name, ApplicationObject *parent, const char *type, bool terminal);
	virtual ~ApplicationObject();

	Database			*database;
	CString				name;
	CString				typeName;
	ApplicationObject	*parent;
	LinkedList			children;
	LinkedList			instances;
	long				options;
	bool				terminal;
	bool				populated;
	bool				sorted;
	CEditor				*editor;
	bool				extensible;
	bool				deletable;
	bool				editable;
	bool				sqlable;
	bool				canXref;
	bool				alive;
	bool				wasOpen;
};

#endif // !defined(AFX_APPLICATIONOBJECT_H__E5A0712D_4371_11D3_AB78_0000C01D2301__INCLUDED_)
