// Sqlda.h: interface for the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SQLDA_H__6C3E2AB9_229F_11D4_98DF_0000C01D2301__INCLUDED_)
#define AFX_SQLDA_H__6C3E2AB9_229F_11D4_98DF_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define DEFAULT_SQLDA_COUNT		20

class Value;
class IscConnection;

class Sqlda  
{
public:
	int findColumn (const char *columnName);
	void setBlob (XSQLVAR *var, Value *value, IscConnection *connection);
	void setValue (int slot, Value *value, IscConnection *connection);
	const char* getTableName (int index);
	static const char* getSqlTypeName (int iscType, int subType);
	static int getSqlType (int iscType, int subType);
	bool isNullable (int index);
	int getScale (int index);
	int getPrecision (int index);
	const char* getColumnName (int index);
	int getDisplaySize (int index);
	int getColumnType (int index);
	void print();
	int getColumnCount();
	void allocBuffer();
	bool checkOverflow();
	void deleteSqlda();
	operator XSQLDA*();
	Sqlda();
	virtual ~Sqlda();

	XSQLDA	*sqlda;
	char	tempSqlda [XSQLDA_LENGTH (DEFAULT_SQLDA_COUNT)];
	char	*buffer;
};

#endif // !defined(AFX_SQLDA_H__6C3E2AB9_229F_11D4_98DF_0000C01D2301__INCLUDED_)
