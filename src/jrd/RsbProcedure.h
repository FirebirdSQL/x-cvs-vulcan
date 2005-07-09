/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbProcedure.h
 *	DESCRIPTION:	Record source block definitions
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
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * Refactored July 1, 2005 by James A. Starkey
 */

#ifndef JRD_RSE_PROCEDURE_H
#define JRD_RSE_PROCEDURE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"

class Procedure;
class Record;
struct dsc;
struct jrd_nod;
struct str;

struct irsb_procedure {
	ULONG		irsb_flags;
	Request*	irsb_req_handle;
	str*		irsb_message;
};

typedef irsb_procedure *IRSB_PROCEDURE;

class RsbProcedure : public RecordSource
{
public:
	RsbProcedure(CompilerScratch *csb, Procedure *proc, jrd_nod *inputsNode, jrd_nod *msgNode);
	virtual ~RsbProcedure(void);
	virtual void open(Request* request, thread_db* tdbb);
	virtual bool get(Request* request, thread_db* tdbb, RSE_GET_MODE mode);
	virtual void close(Request* request, thread_db* tdbb);
	void procAssignment(dsc* from_desc, dsc* flag_desc, UCHAR* msg, dsc* to_desc, SSHORT to_id, Record* record);

	Procedure*	procedure;		// procedure, if appropriate
	jrd_nod		*message;
	jrd_nod		*inputs;
};

#endif

