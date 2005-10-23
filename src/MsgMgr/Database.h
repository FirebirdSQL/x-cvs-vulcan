#ifndef _DATABASE_H
#define _DATABASE_H

class Connection;
class ResultSet;

class Database
{
public:
	Connection	*connection;

	static Database* connect(const char* connectString, const char *account, const char *passwd);
	Database(const char* connectString, Connection* connect);
	~Database(void);
	
	CString		name;
	CString		defaultFacility;
	
	int getFacCode(const char* facility);
	void listMessages(void);
	void genSummary(void);
	void addFacility(void);
	void listFacilities(void);
	void addMessage(void);
	void updateMessage(void);
	void displayResults(CString label, ResultSet* resultSet);
};

#endif
