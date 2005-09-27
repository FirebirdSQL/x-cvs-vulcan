/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbCross.cpp
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
 * Refactored July 11, 2005 by James A. Starkey
 */
 
#include "fbdev.h"
#include "ibase.h"
#include "RsbCross.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "req.h"
#include "ExecutionPathInfoGen.h"

RsbCross::RsbCross(CompilerScratch *csb, int count) : RecordSource(csb, rsb_cross)
{
	rsb_count = count;
	rsbs = new (csb->csb_pool) RecordSource* [count];
}

RsbCross::~RsbCross(void)
{
	delete [] rsbs;
}

void RsbCross::open(Request* request)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	impure->irsb_flags |= irsb_first | irsb_open;
	impure->irsb_flags &= ~(irsb_singular_processed | irsb_checking_singular);
	//record_param* rpb = &request->req_rpb[rsb_stream];
	//rpb->rpb_window.win_flags = 0;
}

bool RsbCross::get(Request* request, RSE_GET_MODE mode)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);

	if (impure->irsb_flags & irsb_first) 
		{
		for (int i = 0; i <rsb_count; i++) 
			{
			rsbs[i]->open(request);
			
			if (!fetchRecord(request, i, mode))
				return FALSE;
			}
			
		impure->irsb_flags &= ~irsb_first;

		return true;
		}

	/* in the case of a project which has been mapped to an index, 
	   we need to make sure that we only return a single record for 
	   each of the leftmost records in the join */

	if (rsb_flags & rsb_project) 
		{
		if (!fetchRecord(request, 0, mode))
			return FALSE;
		}
	else if (!fetchRecord(request, rsb_count - 1, mode))
		return FALSE;

	return true;
}

bool RsbCross::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_cross))
		return false;

	if (!infoGen->putByte((UCHAR) rsb_count))
		return false;
	
	for (int n = 0; n < rsb_count; ++n)
		if (!rsbs[n]->getExecutionPathInfo(request, infoGen))
			return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbCross::close(Request* request)
{
	for (int n = 0; n < rsb_count; ++n)
		rsbs[n]->close(request);
}

bool RsbCross::fetchRecord(Request* request, int n, RSE_GET_MODE mode)
{
#ifndef SCROLLABLE_CURSORS
	mode = RSE_get_forward;
#endif

	RecordSource* sub_rsb = rsbs[n];

	if (sub_rsb->get(request, mode))
		return TRUE;

	/* we have exhausted this stream, so close it; if there is 
	   another candidate record from the n-1 streams to the left, 
	   then reopen the stream and start again from the beginning */

	for (;;)
		{
		sub_rsb->close(request);
		
		if (n == 0 || !fetchRecord(request, n - 1, mode))
			return false;
			
		sub_rsb->open(request);

		if (sub_rsb->get(request, mode))
			return TRUE;
		}
}


void RsbCross::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	for (int n = 0; n < rsb_count; ++n)
		rsbs[n]->findRsbs(stream_list, rsb_list);
}

void RsbCross::pushRecords(Request* request)
{
	for (int n = 0; n < rsb_count; ++n)
		rsbs[n]->pushRecords(request);
}

void RsbCross::popRecords(Request* request)
{
	for (int n = 0; n < rsb_count; ++n)
		rsbs[n]->popRecords(request);
}
