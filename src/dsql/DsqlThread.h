// DsqlThread.h: interface for the DsqlThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DSQLTHREAD_H__797A0F27_6D36_4D28_AA29_C8BF07FD8964__INCLUDED_)
#define AFX_DSQLTHREAD_H__797A0F27_6D36_4D28_AA29_C8BF07FD8964__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class tsql;
class DsqlMemoryPool;

class DsqlThread  
{
public:
	void setMemoryPool (DsqlMemoryPool *pool);
	ISC_STATUS getStatus();
	DsqlThread(ISC_STATUS *statusVector);
	virtual ~DsqlThread();

	tsql thd_context;
	tsql* tdsql;
};

#endif // !defined(AFX_DSQLTHREAD_H__797A0F27_6D36_4D28_AA29_C8BF07FD8964__INCLUDED_)
