// Attachment.cpp: implementation of the Attachment class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Attachment.h"
#include "Database.h"
#include "Connection.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Attachment::Attachment(Database *db)
{
	database = db;
	connection = NULL;
}

Attachment::~Attachment()
{
	if (connection)
		close();

	database->deleteAttachment (this);
}

void Attachment::connect()
{
	connection = database->createConnection();
}

void Attachment::close()
{
	if (connection)
		{
		connection->close();
		connection = NULL;
		}
}
