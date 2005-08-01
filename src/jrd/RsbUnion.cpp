/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbUnion.cpp
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
 * Refactored July 22, 2005 by James A. Starkey
 */
 
#include "firebird.h"
#include "RsbUnion.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/exe_proto.h"

RsbUnion::RsbUnion(CompilerScratch *csb, int streamCount, int nStreams) : RecordSource(csb, rsb_union)
{
	rsb_count = streamCount;
	numberStreams = nStreams;
	rsbs = new (csb->csb_pool) RecordSource* [rsb_count];
	maps = new (csb->csb_pool) jrd_nod* [rsb_count];
	//nodes = new (csb->csb_pool) jrd_nod* [clauseCount];
	streams = new (csb->csb_pool) UCHAR [numberStreams];
	rsb_impure = CMP_impure(csb, sizeof(struct irsb));
}

RsbUnion::~RsbUnion(void)
{
	delete [] rsbs;
	delete [] maps;
	delete [] streams;
}


void RsbUnion::open(Request* request)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	thread_db *tdbb = request->req_tdbb;
	impure->irsb_count = 0;
	record_param* rpb = &request->req_rpb[rsb_stream];
	VIO_record(tdbb, rpb, rsb_format, tdbb->tdbb_default);

	/* Initialize the record number of each stream in the union */

	/***
	RecordSource** ptr = &rsb_arg[rsb_count];
	for (RecordSource** const end = ptr + (USHORT)(long) *ptr;  ++ptr <= end;) 
		request->req_rpb[(USHORT)(long) * ptr].rpb_number.setValue(BOF_NUMBER);
	***/

	for (int n = 0; n < rsb_count; ++n)
		request->req_rpb[streams[n]].rpb_number.setValue(BOF_NUMBER);
		
	rsbs[0]->open(request);
}

bool RsbUnion::get(Request* request, RSE_GET_MODE mode)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	thread_db *tdbb = request->req_tdbb;

	/***
	RecordSource** rsb_ptr = rsb->rsb_arg + impure->irsb_count;

	// March thru the sub-streams (tributaries?) looking for a record 

	while (!(*rsb_ptr)->get(request, RSE_get_forward))
		{
		(*rsb_ptr)->close(request);
		impure->irsb_count += 2;
		
		if (impure->irsb_count >= rsb_count)
			return FALSE;
			
		rsb_ptr += 2;
		(*rsb_ptr)->open(request);
		}
	***/
	
	while (!rsbs[impure->irsb_count]->get(request, RSE_get_forward))
		{
		rsbs[impure->irsb_count]->close(request);
		
		if (++impure->irsb_count >= rsb_count)
			return false;
			
		rsbs[impure->irsb_count]->open(request);
		}
		
	/* We've got a record, map it into the target record */

	//JRD_NOD map = (JRD_NOD) rsb_ptr[1];
	JRD_NOD map = maps[impure->irsb_count];

	for (JRD_NOD *ptr = map->nod_arg, *end = ptr + map->nod_count; ptr < end; ptr++)
		EXE_assignment(tdbb, *ptr);

	return TRUE;
}

void RsbUnion::close(Request* request)
{
	for (int n = 0; n < rsb_count; ++n)
		rsbs[n]->close(request);
}

void RsbUnion::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	stream_list->push(rsb_stream);
		
	if (rsb_list) 
		rsb_list->push(this);
}

void RsbUnion::pushRecords(Request* request)
{
	record_param *rpb = request->req_rpb + rsb_stream;
	saveRecord(request, rpb);
}

void RsbUnion::popRecords(Request* request)
{
	record_param *rpb = request->req_rpb + rsb_stream;
	restoreRecord(rpb);
}
