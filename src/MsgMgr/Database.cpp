#include "stdafx.h"
#include "MsgMgr.h"
#include "Database.h"
#include "Connection.h"
#include "SQLException.h"
#include "PStatement.h"
#include "RSet.h"


Database::Database(const char* connectString, Connection* connect)
{
	connection = connect;
	name = connectString;
}

Database::~Database(void)
{
	if (connection)
		{
		connection->rollback();
		connection->close();
		}
}

Database* Database::connect(const char* connectString, const char *account, const char *password)
{
	Connection *connection = NULL;
	
	try
		{
		connection = createConnection();
		Properties *properties = connection->allocProperties();
		
		if (account[0])
			properties->putValue ("user", account);
		
		if (password[0])
			properties->putValue ("password", password);
			
		connection->openDatabase(connectString, properties);
		delete properties;
		
		return new Database(connectString, connection);
		}
	catch (SQLException& exception)
		{
		if (connection)
			connection->close();
			
		AfxMessageBox(exception.getText());
		
		return NULL;
		}
}


int Database::getFacCode(const char* facility)
{
	PStatement statement = connection->prepareStatement(
		"select fac_code from facilities where facility=?");
	statement->setString(1, facility);
	RSet resultSet = statement->executeQuery();
	
	if (resultSet->next())
		return resultSet->getInt(1);
	
	return 0;
}
