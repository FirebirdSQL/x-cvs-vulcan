#include <stdio.h>
#include "firebird.h"
#include <Connection.h>


namespace IscDbcLibrary {
main (int argc, char **argv)
{
	Connection *connection = NULL;

	try
		{
		connection = createConnection();
		Properties *properties = connection->allocProperties();
                properties->putValue ("user", "sysdba");
                properties->putValue ("password", "masterkey");
                properties->putValue ("client", "firebird32.dll");
		connection->openDatabase ("employee.fdb", properties);
		//connection->openDatabase ("sophie:c:\\harrison\\something.gdb", properties);
		//properties->release();
		delete properties;

		PreparedStatement *statement = connection->prepareStatement (
			"delete from x0");
		statement->executeUpdate();
		connection->rollback();
		
		/***
			"select first_name, last_name, emp_no from employee where first_name = ? for update");
		statement->setCursorName("fred");
		statement->setString (1, "Robert");

		PreparedStatement *update = connection->prepareStatement (
			"update employee set first_Name=? where current of fred");
			
		ResultSet *resultSet = statement->executeQuery();

		while (resultSet->next())
			{
			const char *firstName = resultSet->getString ("first_name");
			const char *lastName = resultSet->getString (2);	// last name
			short empNo = resultSet->getShort (3);					// emp-no
			printf ("%.10s %.15s %d\n", firstName, lastName, empNo);
			update->setString(1, firstName);
			update->executeUpdate();
			}

		resultSet->release();
		***/
		
		statement->release();
		connection->release();
		}
	catch (SQLException& exception)
		{
		printf ("Query failed: %s\n", exception.getText());
		if (connection)
			connection->release();
		return 1;
		}

	return 0;
}
}

main (int argc, char **argv)
{
	//getchar();
	return IscDbcLibrary::main (argc, argv);
}


