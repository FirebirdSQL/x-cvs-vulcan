// IscStatement.h: interface for the IscStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCSTATEMENT_H__C19738B8_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCSTATEMENT_H__C19738B8_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "LinkedList.h"
#include "Sqlda.h"



class IscConnection;
class IscResultSet;

class IscStatement : public Statement  
{
public:
	void setValue(Value *value, XSQLVAR *var);
	void getUpdateCounts();
	virtual int objectVersion();
	void clearResults();
	virtual bool execute();
	void prepareStatement (const char *sqlString);
	void deleteResultSet (IscResultSet *resultSet);
	IscStatement(IscConnection *connect);
	virtual int getUpdateCount();
	virtual bool getMoreResults();
	virtual void setCursorName (const char *name);
	virtual ResultSet* executeQuery (const char *sqlString);
	virtual ResultSet* getResultSet();
	virtual ResultList* search (const char *searchString);
	virtual int executeUpdate (const char *sqlString);
	virtual bool execute (const char *sqlString);
	virtual void close();
	virtual int release();
	virtual void addRef();
	virtual ~IscStatement();

	IscResultSet*	createResultSet();
	LinkedList		resultSets;
	IscConnection	*connection;
	int				useCount;
	int				numberColumns;
	int				resultsCount;
	int				resultsSequence;
	isc_stmt_handle	statementHandle;
	Sqlda			inputSqlda;
	Sqlda			outputSqlda;
	int				updateCount;
	int				updateDelta;
	int				deleteCount;
	int				deleteDelta;
	int				insertCount;
	int				insertDelta;
	int				summaryUpdateCount;
	void reset(void);
};

#endif // !defined(AFX_ISCSTATEMENT_H__C19738B8_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
