// IscConnection.h: interface for the IscConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCCONNECTION_H__C19738B7_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCCONNECTION_H__C19738B7_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "LinkedList.h"
#include "JString.h"	// Added by ClassView

class IscStatement;
class IscDatabaseMetaData;
class Attachment;

class IscConnection : public Connection  
{
public:
	void rollbackAuto();
	void commitAuto();
	virtual CallableStatement* prepareCall (const char *sql);
	virtual int release();
	virtual void addRef();
	virtual int getTransactionIsolation();
	virtual void setTransactionIsolation (int level);
	virtual bool getAutoCommit();
	virtual void setAutoCommit (bool setting);
	void init();
	IscConnection (IscConnection *source);
	virtual Connection* clone();
	virtual int objectVersion();
	virtual Properties* allocProperties();
	JString getInfoString (char *buffer, int item, const char *defaultString);
	int getInfoItem (char *buffer, int item, int defaultValue);
	static JString getIscStatusText (ISC_STATUS *statusVector);
	isc_tr_handle startTransaction();
	void deleteStatement (IscStatement *statement);
	IscConnection();
	virtual ~IscConnection();
	virtual void openDatabase (const char *dbName, Properties *properties);
	virtual void createDatabase (const char *host, const char * dbName, Properties *properties);
	virtual void ping();
	virtual int hasRole (const char *schemaName, const char *roleName);
	//virtual void freeHTML (const char *html);
	virtual Clob* genHTML (Properties *context, long genHeaders);
	virtual bool isConnected();
	virtual Statement* createStatement();
	virtual void prepareTransaction();
	virtual void rollback();
	virtual void commit();
	virtual PreparedStatement* prepareStatement (const char *sqlString);
	virtual void close();
	virtual DatabaseMetaData* getMetaData();

	Attachment		*attachment;
	isc_db_handle	databaseHandle;
	isc_tr_handle	transactionHandle;
	LinkedList		statements;
	IscDatabaseMetaData	*metaData;
	int				transactionIsolation;
	bool			autoCommit;
	int				useCount;
};

#endif // !defined(AFX_ISCCONNECTION_H__C19738B7_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
