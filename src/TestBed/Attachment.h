// Attachment.h: interface for the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ATTACHMENT_H__B96C96D5_0584_11D4_98DB_0000C01D2301__INCLUDED_)
#define AFX_ATTACHMENT_H__B96C96D5_0584_11D4_98DB_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class Database;
class Connection;

class Attachment  
{
public:
	void close();
	void connect();
	Attachment(Database *db);
	virtual ~Attachment();

    Attachment	*next;
	Database	*database;
	Connection	*connection;
};

#endif // !defined(AFX_ATTACHMENT_H__B96C96D5_0584_11D4_98DB_0000C01D2301__INCLUDED_)
