// IscSqlType.h: interface for the IscSqlType class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISCSQLTYPE_H__32C6E499_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_ISCSQLTYPE_H__32C6E499_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class IscSqlType  
{
public:
	void getType (int blrType, int subType, int length);
	IscSqlType(int blrType, int subType, int length);
	virtual ~IscSqlType();

	int			type;
	const char	*typeName;
	int			length;
};

#endif // !defined(AFX_ISCSQLTYPE_H__32C6E499_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
