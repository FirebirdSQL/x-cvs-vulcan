// MsgMgrDoc.h : interface of the CMsgMgrDoc class
//


#pragma once

class Database;

class CMsgMgrDoc : public CDocument
{
protected: // create from serialization only
	CMsgMgrDoc();
	DECLARE_DYNCREATE(CMsgMgrDoc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CMsgMgrDoc();
	
	Database	*database;
	
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	void setDatabase(Database *db);
};


