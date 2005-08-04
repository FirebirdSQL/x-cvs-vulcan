/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbBoolean.cpp
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
 * Refactored July 20, 2005 by James A. Starkey
 */
 
#include "firebird.h"
#include "RsbBoolean.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "req.h"
#include "../jrd/evl_proto.h"

RsbBoolean::RsbBoolean(CompilerScratch *csb, RecordSource* prior_rsb, jrd_nod* node) : RecordSource(csb, rsb_boolean)
{
	rsb_next = prior_rsb;
	boolean = node;
	rsb_any_boolean = NULL;
}

RsbBoolean::~RsbBoolean(void)
{
}

void RsbBoolean::open(Request* request)
{
	rsb_next->open(request);
}

bool RsbBoolean::get(Request* request, RSE_GET_MODE mode)
{
	thread_db *tdbb = request->req_tdbb;

	if (rsb_any_boolean)
		return getAny(request, mode);
		
	UCHAR nullFlag = FALSE;
	
	//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
	while (rsb_next->get(request, mode))
		{
		if (EVL_boolean(request->req_tdbb, boolean))
			return true;


		if (request->req_flags & req_null)
			nullFlag = TRUE;
		}

	if (nullFlag)
		request->req_flags |= req_null;
		
	return FALSE;
}

bool RsbBoolean::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	return false;
}

void RsbBoolean::close(Request* request)
{
	rsb_next->close(request);
}

bool RsbBoolean::getAny(Request* request, RSE_GET_MODE mode)
{
	thread_db *tdbb = request->req_tdbb;
	SSHORT select_value;	/* select for ANY/ALL processing */
	JRD_NOD select_node;	/* ANY/ALL select node pointer */
	JRD_NOD column_node;	/* ANY/ALL column node pointer */

	/* For ANY and ALL clauses (ALL is handled as a negated ANY),
		we must first detect them, and then make sure that the returned
		results are correct.   This mainly entails making sure that
		there are in fact records in the source stream to test against.
		If there were none, the response must be FALSE.
		Also, if the result of the column comparison is always
		NULL, this must also be returned as NULL.   (Note that normally,
		an AND of a NULL and a FALSE would be FALSE, not NULL).

		This all depends on evl.c putting the unoptimized expression
		in the rsb.   The unoptimized expression always has the
		select expression on the left, and the column comparison
		on the right. */

	bool result = true;
	column_node = (JRD_NOD) rsb_any_boolean;
	
	if (request->req_flags & (req_ansi_all | req_ansi_any))
		{
		/* see if there's a select node to work with */

		if (column_node->nod_type == nod_and)
			{
			select_node = column_node->nod_arg[0];
			column_node = column_node->nod_arg[1];
			}
		else
			select_node = NULL;
		}

	if (request->req_flags & req_ansi_any)
		{
		SSHORT any_null;	/* some records null for ANY/ALL */
		SSHORT any_true;	/* some records true for ANY/ALL */
		request->req_flags &= ~req_ansi_any;
		
		if (request->req_flags & req_ansi_not)
			{
			request->req_flags &= ~req_ansi_not;

			/* do NOT ANY */
			/* if the subquery was the empty set
				(numTrue + numFalse + numUnknown = 0)
				or if all were false
				(numTrue + numUnknown = 0),
				NOT ANY is true */

			any_null = FALSE;
			any_true = FALSE;
			
			//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
			while (rsb_next->get(request, mode))
				{
				if (EVL_boolean(tdbb, boolean))
					{
					/* found a TRUE value */

					any_true = TRUE;
					break;
					}

				/* check for select stream and nulls */

				if (!select_node)
					{
					if (request->req_flags & req_null)
						{
						any_null = TRUE;
						break;
						}
					}
				else
					{
					request->req_flags &= ~req_null;
					select_value = EVL_boolean(tdbb, select_node);

					/* see if any record in select stream */

					if (select_value)
						{
						/* see if any nulls */

						request->req_flags &= ~req_null;
						EVL_boolean(tdbb, column_node);

						/* see if any record is null */

						if (request->req_flags & req_null) 
							{
							any_null = TRUE;
							break;
							}
						}
					}
				}
				
			request->req_flags &= ~req_null;
			
			return (any_null || any_true);
			}
		else
			{
			/* do ANY */
			/* if the subquery was true for any comparison, ANY is true */

			result = FALSE;
			
			//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
			while (rsb_next->get(request, mode))
				{
				if (EVL_boolean(tdbb, boolean)) 
					{
					result = TRUE;
					break;
					}
				}
				
			request->req_flags &= ~req_null;
			
			return result;
			}
		}
	else if (request->req_flags & req_ansi_all)
		{
		SSHORT any_false;	/* some records false for ANY/ALL */
		request->req_flags &= ~req_ansi_all;
		
		if (request->req_flags & req_ansi_not)
			{
			request->req_flags &= ~req_ansi_not;

			/* do NOT ALL */
			/* if the subquery was false for any comparison, NOT ALL is true */

			any_false = FALSE;
			
			//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
			while (rsb_next->get(request, mode))
				{
				request->req_flags &= ~req_null;

				/* look for a FALSE (and not null either) */

				if (!EVL_boolean(tdbb, boolean) && !(request->req_flags & req_null))
					{

					/* make sure it wasn't FALSE because there's
						no select stream record */

					if (select_node) 
						{
						request->req_flags &= ~req_null;
						select_value = EVL_boolean(tdbb, select_node);
						
						if (select_value) 
							{
							any_false = TRUE;
							break;
							}
						}
					else 
						{
						any_false = TRUE;
						break;
						}
					}
				}
				
			request->req_flags &= ~req_null;
			
			return !any_false;
			}
		else
			{
			/* do ALL */
			/* if the subquery was the empty set (numTrue + numFalse + numUnknown = 0)
				or if all were true (numFalse + numUnknown = 0), ALL is true */

			any_false = FALSE;
			
			//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
			while (rsb_next->get(request, mode))
				{
				request->req_flags &= ~req_null;

				/* look for a FALSE or null */

				if (!EVL_boolean(tdbb, boolean))
					{
					/* make sure it wasn't FALSE because there's
						no select stream record */

					if (select_node) 
						{
						request->req_flags &= ~req_null;
						select_value = EVL_boolean(tdbb, select_node);
						
						if (select_value) 
							{
							any_false = TRUE;
							break;
							}
						}
					else 
						{
						any_false = TRUE;
						break;
						}
					}
				}
				
			request->req_flags &= ~req_null;
			
			if (any_false)
				return FALSE;
				
			return true;
			}
		}
	
	return false;
}
