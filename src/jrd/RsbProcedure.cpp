/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbProcedure.cpp
 *	DESCRIPTION:	Run time record fetching
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
 
#include "firebird.h"
#include "RsbProcedure.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "Procedure.h"
#include "req.h"
#include "val.h"
#include "../jrd/exe_proto.h"
#include "../jrd/mov_proto.h"

RsbProcedure::RsbProcedure(CompilerScratch *csb, Procedure *proc, jrd_nod *inputsNode, jrd_nod *msgNode)
	 : RecordSource(csb, rsb_procedure)
{
	procedure = proc;
	inputs = inputsNode;
	message = msgNode;
}

RsbProcedure::~RsbProcedure(void)
{
}

void RsbProcedure::open(Request* request)
{
	JRD_NOD *ptr, *end;//, in_message;
	USHORT iml;
	UCHAR *im;

	thread_db* tdbb = request->req_tdbb;
	IRSB_PROCEDURE impure = (IRSB_PROCEDURE) IMPURE (request, rsb_impure);
	record_param* rpb = request->req_rpb + rsb_stream;

	/* get rid of any lingering record */

	
	if (rpb->rpb_record) 
		{
		delete rpb->rpb_record;
		rpb->rpb_record = NULL;
		}

	Request *proc_request = EXE_find_request(tdbb, procedure->findRequest(), FALSE);
	impure->irsb_req_handle = proc_request;
	
	if (inputs) 
		{
		enum req_s saved_state = request->req_operation;

		for (ptr = inputs->nod_arg, end = ptr + inputs->nod_count; ptr < end; ptr++)
			EXE_assignment(tdbb, *ptr);

		request->req_operation = saved_state;
		Format* format = (Format*) message->nod_arg[e_msg_format];
		iml = format->fmt_length;
		im = IMPURE (request, message->nod_impure);
		}
	else 
		{
		iml = 0;
		im = NULL;
		}

	/* req_proc_fetch flag used only when fetching rows, so
	   is set at end of open_procedure (). */

	proc_request->req_flags &= ~req_proc_fetch;
	EXE_start(tdbb, proc_request, request->req_transaction);
	
	if (iml)
		EXE_send(tdbb, proc_request, 0, iml, im);
		
	proc_request->req_flags |= req_proc_fetch;
}

bool RsbProcedure::get(Request* request, RSE_GET_MODE mode)
{
	if (request->req_flags & req_abort)
		return FALSE;

	if (!request->req_transaction)
		return FALSE;

	thread_db* tdbb = request->req_tdbb;
	IRSB_PROCEDURE impure = (IRSB_PROCEDURE) IMPURE (request, rsb_impure);
	
	if (impure->irsb_flags & irsb_singular_processed)
		return FALSE;

	record_param* rpb = request->req_rpb + rsb_stream;
	JRD_REQ proc_request = impure->irsb_req_handle;
	Format* rec_format = procedure->findFormat();
	Format* msg_format = (Format*) (procedure->findOutputMsg())->nod_arg[e_msg_format];
	
	if (!impure->irsb_message)
		{
		const SLONG size = msg_format->fmt_length + ALIGNMENT;
		impure->irsb_message = FB_NEW_RPT(*tdbb->tdbb_default, size) str();
		impure->irsb_message->str_length = size;
		}
		
	UCHAR* om = (UCHAR*) FB_ALIGN((U_IPTR) impure->irsb_message->str_data, ALIGNMENT);
	USHORT oml = impure->irsb_message->str_length - ALIGNMENT;
	Record* record;
	
	if (!rpb->rpb_record) 
		{
		record = rpb->rpb_record = FB_NEW_RPT(*tdbb->tdbb_default, rec_format->fmt_length) Record();
		record->rec_format = rec_format;
		record->rec_length = rec_format->fmt_length;
		}
	else
		record = rpb->rpb_record;

	EXE_receive(tdbb, proc_request, 1, oml, om);

	dsc desc = msg_format->fmt_desc[msg_format->fmt_count - 1];
	desc.dsc_address = (UCHAR *) (om + (long) desc.dsc_address);
	USHORT eos;
	dsc eos_desc (dtype_short, sizeof (SSHORT), &eos);
	MOV_move(&desc, &eos_desc);
	
	if (!eos)
		return FALSE;

	for (int i = 0; i < rec_format->fmt_count; i++)
		procAssignment(&msg_format->fmt_desc[2 * i],
						&msg_format->fmt_desc[2 * i + 1],
						om,
						&rec_format->fmt_desc[i],
						i,
						record);

	return TRUE;
}

void RsbProcedure::close(Request* request)
{
	IRSB_PROCEDURE impure = (IRSB_PROCEDURE) IMPURE (request, rsb_impure);
	Request* proc_request = impure->irsb_req_handle;
	
	if (proc_request) 
		{
		/* bug #7884: at this point the transaction could already have
		   been released, so null it out so as not to dereference it */

		proc_request->req_transaction = NULL;
		//EXE_unwind(request->req_tdbb, proc_request);
		proc_request->unwind();
		proc_request->req_flags &= ~req_in_use;
		impure->irsb_req_handle = 0;
		proc_request->req_attachment = NULL;
		}

	if (impure->irsb_message) 
		{
		delete impure->irsb_message;
		impure->irsb_message = NULL;
		}
}

void RsbProcedure::procAssignment(DSC* from_desc, DSC* flag_desc, UCHAR* msg, DSC* to_desc, SSHORT to_id, Record* record)
{
	SSHORT indicator, l;
	UCHAR *p;

	dsc desc2(dtype_short, sizeof(SSHORT), &indicator);
	dsc desc1 = *flag_desc;
	desc1.dsc_address = msg + (long) flag_desc->dsc_address;
	MOV_move(&desc1, &desc2);
	
	if (indicator) 
		{
		SET_NULL(record, to_id);
		l = to_desc->dsc_length;
		p = record->rec_data + (long) to_desc->dsc_address;
		
		switch (to_desc->dsc_dtype) 
			{
			case dtype_text:
				/* YYY - not necessarily the right thing to do */
				/* YYY for text formats that don't have trailing spaces */
				if (l)
					do
						*p++ = ' ';
					while (--l);
				break;

			case dtype_cstring:
				*p = 0;
				break;

			case dtype_varying:
				*(SSHORT *) p = 0;
				break;

			default:
				do
					*p++ = 0;
				while (--l);
				break;
			}
		to_desc->dsc_flags |= DSC_null;
		}
	else 
		{
		CLEAR_NULL(record, to_id);
		desc1 = *from_desc;
		desc1.dsc_address = msg + (long) desc1.dsc_address;
		desc2 = *to_desc;
		desc2.dsc_address = record->rec_data + (long) desc2.dsc_address;
		
		if (!DSC_EQUIV(&desc1, &desc2))
			MOV_move(&desc1, &desc2);
		else if (desc1.dsc_dtype == dtype_short)
			*((SSHORT *) desc2.dsc_address) = *((SSHORT *) desc1.dsc_address);
		else if (desc1.dsc_dtype == dtype_long)
			*((SLONG *) desc2.dsc_address) = *((SLONG *) desc1.dsc_address);
		else if (desc1.dsc_dtype == dtype_int64)
			*((SINT64 *) desc2.dsc_address) = *((SINT64 *) desc1.dsc_address);
		else if (((U_IPTR) desc1.dsc_address & (ALIGNMENT - 1)) ||
				 ((U_IPTR) desc2.dsc_address & (ALIGNMENT - 1)))
			MOVE_FAST(desc1.dsc_address, desc2.dsc_address, desc1.dsc_length);
		else
			MOVE_FASTER(desc1.dsc_address, desc2.dsc_address, desc1.dsc_length);
		}
}

void RsbProcedure::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	stream_list->push(rsb_stream);
		
	if (rsb_list) 
		rsb_list->push(this);
}

void RsbProcedure::pushRecords(Request* request)
{
	record_param *rpb = request->req_rpb + rsb_stream;
	saveRecord(request, rpb);
}

void RsbProcedure::popRecords(Request* request)
{
	record_param *rpb = request->req_rpb + rsb_stream;
	restoreRecord(rpb);
}
