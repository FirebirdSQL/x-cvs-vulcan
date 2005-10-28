#include "stdafx.h"
#include "MsgMgr.h"
#include "Database.h"
#include "Connection.h"
#include "SQLException.h"
#include "PStatement.h"
#include "RSet.h"
#include "ResultWindow.h"
#include "ListMsgsDialog.h"
#include "AddFacilityDialog.h"
#include "UpdateMsgDialog.h"
#include "AddMsgDialog.h"
#include "QueryDialog.h"
#include "ListHistoryDialog.h"


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

Database* Database::connect(const char* connectString, const char *account, const char *password, const char *role)
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
			
		if (role[0])
			properties->putValue ("role", role);
			
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

void Database::listMessages(void)
{
	try
		{
		ListMsgsDialog dialog;
		dialog.populate(this);
		dialog.facility = defaultFacility;
		
		if (dialog.DoModal() == IDOK)
			{
			defaultFacility = dialog.facility;
			CString sql = "select facility,number,symbol,text from messages m, facilities f "
						"  where m.fac_code=f.fac_code";
			
			if (dialog.facility != ALL)
				sql += " and facility=?";
				
			if (!dialog.containing.IsEmpty())
				sql += " and text containing ?";
				
			switch (dialog.order)
				{
				case number:
					sql += " order by number,facility";
					break;
					
				case symbol:
					sql += " order by symbol";
					break;
					
				case text:
					sql += " order by text";
					break;
				
				default:
					sql += " order by facility, number";
				}
			
			try
				{
				PStatement statement = connection->prepareStatement(sql);
				int n = 1;
				
				if (dialog.facility != ALL)
					statement->setString(n++, dialog.facility);
					
				if (!dialog.containing.IsEmpty())
					statement->setString(n++, dialog.containing);
					
				RSet resultSet = statement->executeQuery();
				displayResults(dialog.facility + " Messages", resultSet);
				}
			catch (SQLException& exception)
				{
				AfxMessageBox(exception.getText());
				}
			}
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void Database::genSummary(void)
{
	try
		{
		PStatement statement = connection->prepareStatement(
			"select facility, max(number), count(*), max(max_number) as last_assigned " 
			" from messages m, facilities f "
			" where m.fac_code=f.fac_code "
			" group by f.facility");
		RSet resultSet = statement->executeQuery();
		displayResults("Message Summary", resultSet);
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void Database::addFacility(void)
{
	try
		{
		AddFacilityDialog dialog;
		PStatement statement = connection->prepareStatement(
			"select max(fac_code + 1) from facilities");
		RSet resultSet = statement->executeQuery();
		resultSet->next();
		dialog.facCode = resultSet->getString(1);
		
		if (dialog.DoModal() == IDOK)
			{
			try
				{
				statement = connection->prepareStatement(
					"insert into facilities(facility,fac_code,max_number) values (?,?,1)");
				int n = 1;
				statement->setString(n++, dialog.facility);
				statement->setString(n++, dialog.facCode);
				statement->executeUpdate();
				connection->commit();
				}
			catch (SQLException& exception)
				{
				AfxMessageBox(exception.getText());
				}
			}
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void Database::listFacilities(void)
{
	try
		{
		PStatement statement = connection->prepareStatement(
			"select facility,fac_code,max_number,last_change from facilities order by fac_code");
		RSet resultSet = statement->executeQuery();
		displayResults("Facilites", resultSet);
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void Database::addMessage(void)
{
	try
		{
		AddMsgDialog dialog;
		dialog.populate(this);
		dialog.facility = defaultFacility;
			
		while (dialog.DoModal() == IDOK)
			{
			int msgNumber;
			int facCode;
			
			defaultFacility = dialog.facility;
			PStatement statement = connection->prepareStatement(
				"select fac_code, max_number from facilities where facility=?");
			statement->setString(1, dialog.facility);
			RSet resultSet = statement->executeQuery();
			
			if (resultSet->next())
				{
				facCode = resultSet->getInt(1);
				msgNumber = resultSet->getInt(2);
				}
			else
				{
				AfxMessageBox("Can't find facility code");
				continue;
				}
			
			statement = connection->prepareStatement(
				"insert into messages (symbol,number,fac_code,module,routine,text,explanation,trans_notes) "
				"  values(?,?,?,?,?,?,?,?)");
			int n = 1;
			statement->setString(n++, dialog.symbol);
			statement->setInt(n++, msgNumber);
			statement->setInt(n++, facCode);
			statement->setString(n++, dialog.module);
			statement->setString(n++, dialog.routine);
			statement->setString(n++, dialog.text);
			statement->setString(n++, dialog.explanation);
			statement->setString(n++, dialog.translationNotes);
			statement->executeUpdate();
			
			statement = connection->prepareStatement(
				"update facilities set max_number=? where fac_code=?");
			statement->setInt(1, msgNumber + 1);
			statement->setInt(2, facCode);
			statement->executeUpdate();
			
			connection->commit();
			return;
			}
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void Database::updateMessage(void)
{
	UpdateMsgDialog dialog;
	dialog.database = this;
	dialog.facility = defaultFacility;
	
	if (dialog.DoModal() == IDOK)
		{
		defaultFacility = dialog.facility;
		
		try
			{
			int facCode = getFacCode(dialog.facility);
			PStatement statement = connection->prepareStatement(
				"update messages set symbol=?,text=?,explanation=?,trans_notes=?"
				"  where fac_code=? and number=?");
			int n = 1; 
			statement->setString(n++, dialog.symbol);
			statement->setString(n++, dialog.text);
			statement->setString(n++, dialog.explanation);
			statement->setString(n++, dialog.notes);
			statement->setInt(n++, facCode);
			statement->setString(n++, dialog.number);
			statement->executeUpdate();
			connection->commit();
			}
		catch (SQLException& exception)
			{
			AfxMessageBox(exception.getText());
			}
		}
}

void Database::displayResults(CString label, ResultSet* resultSet)
{
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS( ResultWindow );
	ResultWindow *window = (ResultWindow*) pRuntimeClass->CreateObject();
	window->LoadFrame (IDR_EDITWINDOW, WS_OVERLAPPEDWINDOW, NULL );
	window->populate (label, resultSet);
	window->ShowWindow (SW_SHOW);
	window->BringWindowToTop();
}

void Database::listHistory(void)
{
	try
		{
		ListHistoryDialog dialog;
		dialog.database = this;
		dialog.facility = defaultFacility;
		dialog.developer = developer;
		
		if (dialog.DoModal() == IDOK)
			{
			defaultFacility = dialog.facility;
			developer = dialog.developer;
			CString sql = "select change_number,change_who,facility,number,change_date,old_text"
						"  from history h, facilities f "
						"  where h.fac_code=f.fac_code";
			
			if (dialog.facility != "All")
				sql += " and h.fac_code=?";
				
			if (!dialog.number.IsEmpty())
				sql += " and h.number=?";
				
			if (dialog.developer != "All")
				sql += " and change_who=?";
				
			switch (dialog.order)
				{
				case hisNumber:
					sql += " order by change_number";
					break;
					
				case hisDate:
					sql += " order by change_date";
					break;
					
				case hisFacility:
					sql += " order by facility";
					break;
				}
			
			try
				{
				PStatement statement = connection->prepareStatement(sql);
				int n = 1;
				
				if (dialog.facility != "All")
					statement->setInt(n++, getFacCode(dialog.facility));
					
				if (!dialog.number.IsEmpty())
					statement->setString(n++, dialog.number);
					
				if (dialog.developer!= "All")
					statement->setString(n++, dialog.developer);
					
				RSet resultSet = statement->executeQuery();
				displayResults(dialog.facility + " History", resultSet);
				}
			catch (SQLException& exception)
				{
				AfxMessageBox(exception.getText());
				}
			}
		}
	catch (SQLException& exception)
		{
		AfxMessageBox(exception.getText());
		}
}

void Database::sqlQuery(void)
{
	QueryDialog dialog;
	dialog.query = query;
	
	while (dialog.DoModal() == IDOK)
		try
			{
			PStatement statement = connection->prepareStatement(dialog.query);
			RSet resultSet = statement->executeQuery();
			displayResults ("Result Set", resultSet);
			query = dialog.query;
			return;
			}
		catch(SQLException& exception)
			{
			AfxMessageBox(exception.getText());
			}
		
}
