// ArgThread.h: interface for the ArgThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARGTHREAD_H__E11366A8_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ARGTHREAD_H__E11366A8_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Arg.h"

class ArgThread : public Arg  
{
public:
	ArgThread(const char *name);
	virtual ~ArgThread();

};

#endif // !defined(AFX_ARGTHREAD_H__E11366A8_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
