/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ExecuteStatement.h
 *	DESCRIPTION:	Dynamic SQL statements execution
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Alex Peshkoff, 2003
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 */

#ifndef JRD_DynExec_H
#define JRD_DynExec_H

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "../include/fb_vector.h"
#include "../jrd/exe.h"
#include "../jrd/ibase.h"

#define MAX_CALLBACKS	50

class ExecuteStatement {
private:
	isc_handle Attachment;
	isc_handle Transaction;
	isc_handle Statement;
	XSQLDA	* Sqlda;
	SCHAR	* Buffer;
	bool	SingleMode;
	TEXT	* StartOfSqlOperator;
	XSQLDA	* MakeSqlda(thread_db* tdbb, SSHORT n);
	ISC_STATUS	ReMakeSqlda(ISC_STATUS *vector, thread_db* tdbb);
	ULONG	ParseSqlda(void);
public:
	void Open(thread_db* tdbb, JRD_NOD sql, SSHORT nVars, bool SingleTon);
	bool Fetch(thread_db* tdbb, JRD_NOD * FirstVar);
	void Close(thread_db* tdbb);
};

#endif // JRD_DynExec_H
