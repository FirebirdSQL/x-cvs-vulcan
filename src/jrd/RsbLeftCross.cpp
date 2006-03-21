/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbLeftCross.cpp
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
 * Refactored July 21, 2005 by James A. Starkey
 */
 
#include "fbdev.h"
#include "ibase.h"
#include "RsbLeftCross.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "req.h"
#include "../jrd/vio_proto.h"
#include "../jrd/evl_proto.h"
#include "Relation.h"
#include "RsbBoolean.h"
#include "ExecutionPathInfoGen.h"

RsbLeftCross::RsbLeftCross(CompilerScratch *csb, jrd_nod *inBool, RecordSource *inRsb, jrd_nod *outBool, RecordSource *outRsb) 
		: RecordSource(csb, rsb_left_cross)
{
	innerBoolean = inBool;
	outerBoolean = outBool;
	innerRsb = inRsb;
	outerRsb = outRsb;
}

RsbLeftCross::~RsbLeftCross(void)
{
}

void RsbLeftCross::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	outerRsb->open(request);
	impure->irsb_flags &= ~(irsb_first | irsb_in_opened | irsb_join_full);
	impure->irsb_flags |= irsb_mustread;

	/* Allocate a record block for each union/aggregate/procedure
		stream in the right sub-stream.  The block will be needed
		if we join to nulls before opening the rsbs */

	for (RsbStack::iterator stack(*(rsb_left_rsbs));  stack.hasData(); ++stack)
		VIO_record(tdbb, &request->req_rpb[stack.object()->rsb_stream],
				   stack.object()->rsb_format, tdbb->tdbb_default);
}

/**************************************
 *
 *	f e t c h _ l e f t
 *
 **************************************
 *
 * Functional description
 *	Get records for a left outer join.  Records are read
 *	from the left sub-stream when the right sub-stream is
 *	dry or when it is not yet open.  When the left 
 *	sub-stream's boolean is true, open the right sub-stream
 *	and read a record.  When the right sub-stream becomes dry,
 *	close it, and if nothing has been joined to the left
 *	sub-stream's current record, join a null valued right
 *	sub-stream record.  When the left sub-stream is dry,
 *	the outer join is finished, so return FALSE.
 *
 **************************************/

bool RsbLeftCross::get(Request* request, RSE_GET_MODE mode)
{
	thread_db *tdbb = request->req_tdbb;
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	
	if (!(impure->irsb_flags & irsb_join_full))
		{
		while (true)
			{
			if (impure->irsb_flags & irsb_mustread)
				{
				//if (!get_record(request, tdbb, outerRsb, NULL, RSE_get_forward)) 
				if (!outerRsb->get(request, RSE_get_forward))
					{
					if (rsb_left_inner_streams->isEmpty())
						return false;

					/* We have a full outer join.  Open up the inner stream
					   one more time. */

					//RSE_close(tdbb, outerRsb);
					outerRsb->close(request);
					impure->irsb_flags |= irsb_join_full;
					//RSE_open(tdbb, innerRsb);
					outerRsb->open(request);
					break;
					}
					
				if (outerBoolean && !EVL_boolean(tdbb, (jrd_nod*) outerBoolean)) 
					{
					/* The boolean pertaining to the left sub-stream is false
					   so just join sub-stream to a null valued right sub-stream */
					joinToNulls(request, rsb_left_streams);
					return TRUE;
					}
					
				impure->irsb_flags &= ~(irsb_mustread | irsb_joined);
				impure->irsb_flags |= irsb_in_opened;
				//RSE_open(tdbb, innerRsb);
				innerRsb->open(request);
				}

			//while (get_record(request, tdbb, innerRsb, NULL, RSE_get_forward))
			while (innerRsb->get(request, RSE_get_forward))
				if (!innerBoolean
						|| EVL_boolean(tdbb,(JRD_NOD) innerBoolean))
					{
					impure->irsb_flags |= irsb_joined;
					return TRUE;
					}

			//RSE_close(tdbb, innerRsb);
			innerRsb->close(request);
			impure->irsb_flags |= irsb_mustread;
			
			if (!(impure->irsb_flags & irsb_joined))
				{
				/* The current left sub-stream record has not been joined
				   to anything.  Join it to a null valued right sub-stream */
				joinToNulls(request, rsb_left_streams);
				return TRUE;
				}
			}
		}

	/* Continue with a full outer join. */

	//RecordSource* full = innerRsb;
	//full = (full->rsb_type == rsb_boolean) ? full->rsb_next : full;
	RecordSource* full = (innerRsb->rsb_type == rsb_boolean) ? innerRsb->rsb_next : innerRsb;

	if (impure->irsb_flags & irsb_in_opened)
		{
		/* The inner stream was opened at some point.  If it doesn't have a
		   boolean, then all of its records have been returned.  Otherwise,
		   find the records that haven't been. */
		BOOLEAN found;
		
		do {
			if (!full->get(request, RSE_get_forward))
				return FALSE;
				
			outerRsb->open(request);
			
			while( (found = outerRsb->get(request, RSE_get_forward)) )
				{
				if ((!outerBoolean || EVL_boolean(tdbb, outerBoolean)) &&
					(!innerBoolean || EVL_boolean(tdbb, innerBoolean)) &&
					(full == innerRsb || EVL_boolean(tdbb, ((RsbBoolean*) innerRsb)->boolean)))
					break;
				}
			outerRsb->close(request);
			} while (found);
		}
	else if (!full->get(request, RSE_get_forward))
		return FALSE;

	joinToNulls(request, rsb_left_inner_streams);

	return TRUE;
}

bool RsbLeftCross::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_left_cross))
		return false;

	if (!infoGen->putByte(2))
		return false;
	
	if (!outerRsb->getExecutionPathInfo(request, infoGen))
		return false;

	if (!innerRsb->getExecutionPathInfo(request, infoGen))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbLeftCross::close(Request* request)
{
	outerRsb->close(request);
	innerRsb->close(request);
}

void RsbLeftCross::joinToNulls(Request* request, StreamStack* stream)
{
	thread_db *tdbb = request->req_tdbb;

	for (StreamStack::iterator stack(*stream); stack.hasData(); ++stack)
		{
		record_param* rpb = &request->req_rpb[stack.object()];

		/* Make sure a record block has been allocated.  If there isn't
		   one, first find the format, then allocate the record block */

		Record* record = rpb->rpb_record;
		
		if (!record)
			{
			Format* format = rpb->rpb_relation->rel_current_format;
			
			if (!format)
				format = rpb->rpb_relation->getFormat(tdbb, rpb->rpb_format_number);
						 
			record = VIO_record(tdbb, rpb, format, tdbb->tdbb_default);
			}

        record->rec_fmt_bk = record->rec_format;
		record->rec_format = NULL;
		}
}

void RsbLeftCross::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	innerRsb->findRsbs(stream_list, rsb_list);
	outerRsb->findRsbs(stream_list, rsb_list);
}


void RsbLeftCross::pushRecords(Request* request)
{
	outerRsb->pushRecords(request);
	innerRsb->pushRecords(request);
}

void RsbLeftCross::popRecords(Request* request)
{
	outerRsb->popRecords(request);
	innerRsb->popRecords(request);
}
