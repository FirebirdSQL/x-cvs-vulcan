// TreeNode.h: interface for the TreeNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TREENODE_H__E5A0712E_4371_11D3_AB78_0000C01D2301__INCLUDED_)
#define AFX_TREENODE_H__E5A0712E_4371_11D3_AB78_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "LinkedList.h"

enum TreeImage {
    treeImageNone = -1,
    treeImageDatabase = 0,
	treeImageTable,
	treeImageColumn,
	treeImageIndex,
	treeImageFolder,
	treeImageTemplate,
	treeImageApplication,
	};

class ApplicationObject;

class TreeNode  
{
public:
	virtual void reopen();
	virtual bool doOption (long option);
	virtual long getOptions();
	virtual void dropFile (const char *fileName);
	virtual void refreshAll();
	//virtual void search();
	virtual void findReferences();
	virtual void drop (TreeNode *node);
	virtual bool dropEnable (TreeNode *dragObject);
	void buildAll();
	bool changePending();
	void forceExpansion();
	void openPath (const char *name);
	//void serialOpen (CString parentName, CArchive &ar);
	void childDeleted (TreeNode *child);
	virtual void extend();
	virtual void edit();
	//virtual void doSql();
	virtual void deleteObject();
	virtual UINT getMenu();
	virtual BOOL select();
	virtual void popupMenu(CWnd *window, CPoint& point);
	virtual void setName (const char *newName);
	virtual void setAttributes();
	virtual void populate();
	virtual void addChild (TreeNode * child);
	//virtual void zapChildren();
	virtual void deleteItem();
	//virtual void deleteChild (TreeNode * child);
	virtual void deleteChildren();
	virtual void close();
	virtual void expand (UINT state);
	virtual bool doubleClick();
	virtual TreeNode* findNode (HTREEITEM itm);
	void init(TreeNode *prnt, ApplicationObject *obj, bool terminal);
	virtual void update();
	virtual void create();

	TreeNode (CTreeCtrl *control, ApplicationObject *obj);
	TreeNode(TreeNode *parent, ApplicationObject *obj, bool terminal);
	virtual ~TreeNode();

	CString		name;
	LinkedList	children;
	TreeNode	*parent;
	bool		expanded;
	bool		sorted;
	bool		atEnd;
	bool		terminal;
	bool		bold;
	bool		empty;
	bool		extensible;
	bool		deletable;
	bool		editable;
	bool		sqlable;
	bool		canXref;
	CTreeCtrl*	treeControl;
	HTREEITEM	item;
	ApplicationObject	*object;
	TreeImage	image;
	TreeImage	selectedImage;
};

#endif // !defined(AFX_TREENODE_H__E5A0712E_4371_11D3_AB78_0000C01D2301__INCLUDED_)
