// ArgConnection.h: interface for the ArgConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARGCONNECTION_H__E11366A7_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ARGCONNECTION_H__E11366A7_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Arg.h"

class Connection;


class ArgConnection : public Arg  
{
public:
	ArgConnection(const char *name, Connection *cnct);
	virtual ~ArgConnection();

	Connection	*connection;
};

#endif // !defined(AFX_ARGCONNECTION_H__E11366A7_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
