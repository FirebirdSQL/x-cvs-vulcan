// Database.cpp: implementation of the Database class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Database.h"
#include "Connection.h"
#include "Attachment.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Database::Database()
{
	attachments = NULL;
}

Database::~Database()
{
	while (attachments)
		delete attachments;
}


Attachment* Database::createAttachment()
{
	Attachment *attachment = new Attachment (this);
	attachment->next = attachments;
	attachments = attachment;
	attachment->connect();

	return attachment;
}

void Database::deleteAttachment(Attachment * attachment)
{
	for (Attachment **ptr = &attachments; *ptr; ptr = &(*ptr)->next)
		if (*ptr == attachment)
			{
			*ptr = attachment->next;
			break;
			}
}

Connection* Database::createConnection()
{
	Connection *connection = createConnection();
	//connection->openDatabase (attachString, account, password);
	Properties *properties = connection->allocProperties();
	properties->putValue ("user", account);
	properties->putValue ("password", password);
	connection->openDatabase (attachString, properties);
	delete properties;

	return connection;
}
