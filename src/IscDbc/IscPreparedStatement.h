// IscPreparedStatement.h: interface for the IscPreparedStatement class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCPREPAREDSTATEMENT_H__C19738B9_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_ISCPREPAREDSTATEMENT_H__C19738B9_1C87_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "IscStatement.h"
#include "Values.h"

class IscConnection;
class IscStatementMetaData;

class IscPreparedStatement : public IscStatement, public PreparedStatement
{
public:
	virtual int objectVersion();
	virtual void setClob (int index, Clob *value);
	virtual StatementMetaData* getStatementMetaData();
	virtual ~IscPreparedStatement();
	virtual bool		execute (const char *sqlString);
	virtual ResultSet*	executeQuery (const char *sqlString);
	virtual int			getUpdateCount();
	virtual bool		getMoreResults();
	virtual void		setCursorName (const char *name);
	virtual ResultSet*	getResultSet();
	virtual ResultList* search (const char *searchString);
	virtual int			executeUpdate (const char *sqlString);
	virtual void		close();
	virtual int			release();
	virtual void		addRef();

	virtual void		setNull (int index, int type);
	virtual void		setString(int index, const char * string);
	virtual void		setByte (int index, char value);
	virtual void		setShort (int index, short value);
	virtual void		setInt (int index, long value);
	virtual void		setLong (int index, QUAD value);
	virtual void		setFloat (int index, float value);
	virtual void		setDouble (int index, double value);
	virtual void		setDate (int index, DateTime value);
	virtual void		setTimestamp (int index, TimeStamp value);
	virtual void		setTime (int index, Time value);
	virtual void		setBlob (int index, Blob *value);
	virtual void		setBytes (int index, int length, const void *bytes);

	virtual int			executeUpdate();
	virtual bool		execute();
	virtual ResultSet*	executeQuery();

	void				getInputParameters();
	virtual void		prepare (const char *sqlString);
	IscPreparedStatement(IscConnection *connect);
	Value* getParameter (int index);

	Values		parameters;
	IscStatementMetaData	*statementMetaData;
};

#endif // !defined(AFX_ISCPREPAREDSTATEMENT_H__C19738B9_1C87_11D4_98DF_0000C01D2301__INCLUDED_)
