// ArgResultSet.h: interface for the ArgResultSet class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARGRESULTSET_H__E11366AA_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ARGRESULTSET_H__E11366AA_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Arg.h"

class ResultSet;


class ArgResultSet : public Arg  
{
public:
	ArgResultSet(const char *name, ResultSet *results);
	virtual ~ArgResultSet();

	ResultSet	*resultSet;
};

#endif // !defined(AFX_ARGRESULTSET_H__E11366AA_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
