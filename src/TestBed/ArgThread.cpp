// ArgThread.cpp: implementation of the ArgThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "ArgThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ArgThread::ArgThread(const char *name) : Arg (name, argThread)
{

}

ArgThread::~ArgThread()
{

}
