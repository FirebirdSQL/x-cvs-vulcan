// ArgConnection.cpp: implementation of the ArgConnection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ArgConnection.h"
#include "Connection.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArgConnection::ArgConnection(const char *name, Connection *cnct) : Arg (name, argConnection)
{
	connection = cnct;
}

ArgConnection::~ArgConnection()
{

}
