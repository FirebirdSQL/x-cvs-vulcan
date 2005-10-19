#ifndef _DATABASE_H
#define _DATABASE_H

class Connection;

class Database
{
public:
	Connection	*connection;

	static Database* connect(const char* connectString, const char *account, const char *passwd);
	Database(const char* connectString, Connection* connect);
	~Database(void);
	
	CString		name;
};

#endif
