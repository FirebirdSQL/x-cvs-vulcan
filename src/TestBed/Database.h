// Database.h: interface for the Database class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASE_H__B96C96D4_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_DATABASE_H__B96C96D4_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Connection;
class Attachment;

class Database  
{
public:
	Connection* createConnection();
	void deleteAttachment (Attachment *attachment);
	Attachment* createAttachment();
	Database();
	virtual ~Database();

	CString		attachString;
	CString		password;
	CString		account;
	Attachment	*attachments;
};

#endif // !defined(AFX_DATABASE_H__B96C96D4_0584_11D4_98DB_0000C01D2301__INCLUDED_)
