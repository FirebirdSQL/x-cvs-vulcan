// DsqlThread.cpp: implementation of the DsqlThread class.
//
//////////////////////////////////////////////////////////////////////

#include "fbdev.h"
#include "dsql.h"
#include "DsqlThread.h"
#include "thd_proto.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DsqlThread::DsqlThread(ISC_STATUS *statusVector)
{
	tdsql = &thd_context;
	THD_put_specific ((THDD) tdsql, THDD_TYPE_TSQL);
	tdsql->tsql_thd_data.thdd_type = THDD_TYPE_TSQL;
	tdsql->tsql_status = statusVector;
	tdsql->tsql_default = NULL;
}

DsqlThread::~DsqlThread()
{
	THD_restore_specific(THDD_TYPE_TSQL);
}

ISC_STATUS DsqlThread::getStatus()
{
	return tdsql->tsql_status[1];
}

void DsqlThread::setMemoryPool(DsqlMemoryPool *pool)
{
	tdsql->tsql_default = pool;
}
