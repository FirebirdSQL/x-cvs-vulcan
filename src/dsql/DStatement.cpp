// DStatement.cpp: implementation of the DStatement class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "common.h"
#include "DStatement.h"
#include "dsql.h"
#include "CStatement.h"
#include "thd_proto.h"
#include "errd_proto.h"
#include "jrd_proto.h"
#include "gds_proto.h"
#include "sch_proto.h"
#include "status.h"
#include "inf_proto.h"
#include "iberror.h"
#include "CStatement.h"
#include "Attachment.h"
#include "ibase.h"
#include "InfoGen.h"
#include "ThreadData.h"
#include "Cursor.h"
#include "jrd_proto.h"
#include "align.h"
#include "dsc.h"
#include "JrdMove.h"
#include "BlrParse.h"
#include "ibase.h"
#include "OSRIException.h"
#include "ddl_proto.h"
#include "Database.h"
#include "InfoGen.h"

static const UCHAR recordInfo[] = 
	{
	isc_info_req_update_count, 
	isc_info_req_delete_count,
	isc_info_req_select_count, 
	isc_info_req_insert_count
	};

static const UCHAR explainInfo[] = 
	{
	isc_info_access_path
	};


//static int initialized; // = DStatement::staticInitialization();

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DStatement::DStatement(Attachment *attach)
{
	ThreadData thread (NULL);
	CStatement::staticInitialization();
	attachment = attach;
	statement = NULL;
	cursor = NULL;
	transaction = NULL;
	//request = NULL;
	req_flags = 0;
	sendMessage = NULL;
	receiveMessage = NULL;
	requestInstantiation = 0;
	parent = NULL;
	offspring = NULL;
	sibling = NULL;
	singletonFetched = false;
}

DStatement::~DStatement()
{
	closeStatement();
}


ISC_STATUS DStatement::prepare(ISC_STATUS *statusVector, Transaction *trans, int sqlLength, const TEXT *sql, int dialect, int itemLength, const UCHAR *items, int bufferLength, UCHAR *buffer)
{
	ThreadData thread (statusVector, attachment);

	try
		{
		if (!sql)
			ERRD_post(isc_sqlerr, isc_arg_number, -104, 
				isc_arg_gds, isc_command_end_err, 0);

		/*** I don't think this is required by any standard.  jas 10/20/04
		 *** One second thought, maybe it is required...
		if (req_flags & REQ_cursor_open)
			ERRD_post(isc_sqlerr, isc_arg_number, -519,
				  isc_arg_gds, isc_dsql_open_cursor_request, 0);
		***/
		
		reset();
		transaction = trans;
		statement = new CStatement (attachment);
		thread.setDsqlPool (statement->pool);
		thread.threadData->tdbb_transaction = trans;
		statement->prepare (thread, sqlLength, sql, dialect);
		
		if (statement->parent)
			{
			Cursor *cursor = attachment->findCursor(statement->cursorName);
			
			if (!cursor || !(parent = cursor->statement) || parent->statement != statement->parent)
				ERRD_post(isc_sqlerr, isc_arg_number, -510,
						isc_arg_gds, isc_dsql_cursor_update_err, 0);
			
			cursor->statement->addChild(this);
			}
			
		switch (statement->req_type)
			{
			case REQ_SELECT:
			case REQ_SELECT_UPD: 
			case REQ_INSERT: 
			case REQ_DELETE: 
			case REQ_UPDATE:
			case REQ_UPDATE_CURSOR: 
			case REQ_DELETE_CURSOR:
			case REQ_EXEC_PROCEDURE:
				instantiateRequest (thread);
				break;
			
			case REQ_DDL: 
				if (statement->ddlNode->nod_type == nod_def_database)
						ERRD_post(isc_sqlerr, isc_arg_number, -530,
					  		isc_arg_gds, isc_dsql_crdb_prepare_err, 0);
			
			case REQ_COMMIT: 
			case REQ_ROLLBACK: 
			case REQ_EMBED_SELECT:
			case REQ_START_TRANS: 
			case REQ_GET_SEGMENT: 
			case REQ_PUT_SEGMENT: 
			case REQ_COMMIT_RETAIN: 
			case REQ_SET_GENERATOR: 
			case REQ_SAVEPOINT:
			default:
				break;
			}

		return getSqlInfo (statusVector, itemLength, items, bufferLength, buffer);
		}
	catch(OSRIException& exception)
		{
		reset();
		return exception.copy (statusVector);
		}
	
	return FB_SUCCESS;
}

void DStatement::reset()
{
	closeStatement();
}

ISC_STATUS DStatement::getSqlInfo(ISC_STATUS *statusVector, int itemsLength, const UCHAR *items, int info_length, UCHAR *info)
{
	if (itemsLength == 0)
		return returnSuccess (statusVector);

	ThreadData thread (statusVector, attachment);
	UCHAR buffer[256];
	USHORT length, number, first_index;

    try
		{
		memset(buffer, 0, sizeof(buffer));
		const UCHAR* const end_items = items + itemsLength;
		const UCHAR* const end_info = info + info_length;

		// CVC: Is it the idea that this pointer remains with its previous value
		// in the loop or should it be made NULL in each iteration?

		dsql_msg** message = NULL;
		first_index = 0;

		while (items < end_items && *items != isc_info_end)
			{
			UCHAR item = *items++;
			switch (item)
				{
				case isc_info_sql_select:
				case isc_info_sql_bind:
					message = (item == isc_info_sql_select) ?
						&statement->receiveMessage : &statement->sendMessage;
					if (info + 1 >= end_info) 
						{
						*info = isc_info_truncated;
						return returnSuccess(statusVector);
						}
					*info++ = item;
					break;
				
				case isc_info_sql_stmt_type:
					switch (statement->req_type) 
						{
						case REQ_SELECT:
						case REQ_EMBED_SELECT:
							number = isc_info_sql_stmt_select;
							break;
							
						case REQ_SELECT_UPD:
							number = isc_info_sql_stmt_select_for_upd;
							break;
							
						case REQ_DDL:
							number = isc_info_sql_stmt_ddl;
							break;
							
						case REQ_GET_SEGMENT:
							number = isc_info_sql_stmt_get_segment;
							break;
							
						case REQ_PUT_SEGMENT:
							number = isc_info_sql_stmt_put_segment;
							break;
							
						case REQ_COMMIT:
						case REQ_COMMIT_RETAIN:
							number = isc_info_sql_stmt_commit;
							break;
							
						case REQ_ROLLBACK:
							number = isc_info_sql_stmt_rollback;
							break;
							
						case REQ_START_TRANS:
							number = isc_info_sql_stmt_start_trans;
							break;
							
						case REQ_INSERT:
							number = isc_info_sql_stmt_insert;
							break;
							
						case REQ_UPDATE:
						case REQ_UPDATE_CURSOR:
							number = isc_info_sql_stmt_update;
							break;
							
						case REQ_DELETE:
						case REQ_DELETE_CURSOR:
							number = isc_info_sql_stmt_delete;
							break;
							
						case REQ_EXEC_PROCEDURE:
							number = isc_info_sql_stmt_exec_procedure;
							break;
							
						case REQ_SET_GENERATOR:
							number = isc_info_sql_stmt_set_generator;
							break;
							
						case REQ_SAVEPOINT:
							number = isc_info_sql_stmt_savepoint;
							break;
							
						default:
							number = 0;
							break;
						}
					length = convert(number, buffer);
					info = put_item(item, length, buffer, info, end_info);
					if (!info)
						return returnSuccess(statusVector);
					break;
			
			case isc_info_sql_sqlda_start:
				length = *items++;
				first_index = static_cast<USHORT> (gds__vax_integer (items, length));
				items += length;
				break;
			
			case isc_info_sql_batch_fetch:
				number = (statement->flags & REQ_no_batch)? 0 : 1;
				length = convert(number, buffer);
				if (!(info = put_item(item, length, buffer, info, end_info)))
					return returnSuccess(statusVector);
				break;
			
			case isc_info_sql_records: 
				if (statement && statement->request)
					{
					length = getRequestInfo(thread, sizeof(buffer), buffer);
					if (length && !(info = put_item(item, length, buffer, info, end_info))) 
						return returnSuccess(statusVector);
					}
				else
					{
					UCHAR tempBuffer [128];
					InfoGen gen (tempBuffer, sizeof (tempBuffer));
					
					for (int n = 0; n < sizeof (recordInfo); ++n)
						gen.putInt (recordInfo [n], 0);
						
					length = gen.fini();
					
					if (!(info = put_item(item, length, tempBuffer, info, end_info)))
						return returnSuccess(statusVector);
					}
				break;
			
			case isc_info_sql_get_plan: 
				{
				/* be careful, get_plan_info() will reallocate the buffer to a
				  larger size if it is not big enough */

				UCHAR *buffer_ptr = buffer;
				length = getPlanInfo(thread, sizeof(buffer), &buffer_ptr);

				if (length)
					info = put_item(item, length, buffer_ptr, info, end_info);

				if (length > sizeof(buffer))
					gds__free(buffer_ptr);

				if (!info)
					return returnSuccess(statusVector);
				}
				break;
			
			default:
				if (!message || (item != isc_info_sql_num_variables && item != isc_info_sql_describe_vars))
					{
					buffer[0] = item;
					item = isc_info_error;
					length = 1 + convert(isc_infunk, buffer + 1);
					if (!(info = put_item(item, length, buffer, info, end_info)))
						return returnSuccess(statusVector);
					}
				else
					{
					number = (*message) ? (*message)->msg_index : 0;
					length = convert(number, buffer);
					if (!(info = put_item(item, length, buffer, info, end_info)))
						return returnSuccess(statusVector);
					if (item == isc_info_sql_num_variables)
						continue;

					const UCHAR* end_describe = items;
					while (end_describe < end_items &&
				   		*end_describe != isc_info_end &&
				   		*end_describe != isc_info_sql_describe_end) 
						end_describe++;

					info = getVariableInfo(*message, items, end_describe, info, end_info, first_index);
					if (!info) 
						return returnSuccess (statusVector);

					items = end_describe;
					if (*items == isc_info_sql_describe_end)
						items++;
					}
				}
			}

		*info++ = isc_info_end;
		}
	catch(OSRIException& exception)
		{
		return exception.copy (statusVector);
		}

	return returnSuccess (statusVector);
}

ISC_STATUS DStatement::returnSuccess(ISC_STATUS *statusVector)
{
	ISC_STATUS* p = statusVector; //tdsql->tsql_status;
	*p++ = isc_arg_gds;
	*p++ = FB_SUCCESS;

	// do not overwrite warnings
	if (*p != isc_arg_warning)
		*p = isc_arg_end;

	return FB_SUCCESS;
}

int DStatement::convert(int number, UCHAR* buffer)
{
	const UCHAR* p;

#ifndef WORDS_BIGENDIAN
	const SLONG n = number;
	p = (UCHAR *) & n;
	*buffer++ = *p++;
	*buffer++ = *p++;
	*buffer++ = *p++;
	*buffer++ = *p++;

#else

	p = (UCHAR *) (&number + 1);
	*buffer++ = *--p;
	*buffer++ = *--p;
	*buffer++ = *--p;
	*buffer++ = *--p;

#endif

	return 4;
}

UCHAR* DStatement::put_item(UCHAR item, int length, const UCHAR *string, UCHAR *ptr, const UCHAR *end)
{
	if (ptr + length + 3 >= end) 
		{
		*ptr = isc_info_truncated;
		return NULL;
		}

	*ptr++ = item;
	*ptr++ = (UCHAR)length;
	*ptr++ = length >> 8;

	for (; length; --length)
		*ptr++ = *string++;

	return ptr;
}

int DStatement::getRequestInfo(tdbb *threadData, int bufferLength, UCHAR *buffer)
{
	INF_request_info(threadData, statement->request, recordInfo, sizeof(recordInfo), buffer, bufferLength);

	UCHAR* data = buffer;

	//request->req_updates = request->req_deletes = 0;
	//request->req_selects = request->req_inserts = 0;

	UCHAR p;
	while ((p = *data++) != isc_info_end) 
		{
		int data_length = gds__vax_integer(data, 2);
		data += 2;
		/***
		switch (p) 
			{
			case isc_info_req_update_count:
				request->req_updates =
					gds__vax_integer(reinterpret_cast<UCHAR*>(data),
									 data_length);
				break;

			case isc_info_req_delete_count:
				request->req_deletes =
					gds__vax_integer(reinterpret_cast<UCHAR*>(data),
									 data_length);
				break;

			case isc_info_req_select_count:
				request->req_selects =
					gds__vax_integer(reinterpret_cast<UCHAR*>(data),
									 data_length);
				break;

			case isc_info_req_insert_count:
				request->req_inserts =
					gds__vax_integer(reinterpret_cast<UCHAR*>(data),
									 data_length);
				break;

			default:
				break;
			}
		***/
		data += data_length;
		}

	return data - buffer;
}

int DStatement::getPlanInfo(tdbb *threadData, int bufferLength, UCHAR **bufferPtr)
{
	UCHAR explain_buffer[256];
	int buffer_length = bufferLength;
	//TSQL tdsql = GET_THREAD_DATA;
	memset(explain_buffer, 0, sizeof(explain_buffer));
	UCHAR* explain_ptr = explain_buffer;
	UCHAR* buffer_ptr = *bufferPtr;

	// get the access path info for the underlying request from the engine 

	/***
	THREAD_EXIT;
	ISC_status s = isc_request_info(tdsql->tsql_status,
						 &request->req_handle,
						 0,
						 sizeof(explain_info),
						 explain_info,
						 sizeof(explain_buffer), explain_buffer);
	THREAD_ENTER;

	if (s)
		return 0;
	***/
	
	if (!statement->request)
		return 0;
		
	INF_request_info(threadData, statement->request, 
					 explainInfo, sizeof(explainInfo), explain_buffer, sizeof(explain_buffer));

	if (*explain_buffer == isc_info_truncated) 
		{
		explain_ptr = (UCHAR *) gds__alloc(BUFFER_XLARGE);
		// CVC: Added test for memory exhaustion here.
		// Should we throw an exception or simply return 0 to the caller?
		if (!explain_ptr)
			return 0;
		/***
		THREAD_EXIT;
		s = isc_request_info(tdsql->tsql_status,
							 &request->req_handle, 0,
							 sizeof(explain_info),
							 explain_info,
							 BUFFER_XLARGE, explain_ptr);
		THREAD_ENTER;

		if (s) 
			{
			// CVC: Before returning, deallocate the buffer!
			gds__free(explain_ptr);
			return 0;
			}
		***/
		INF_request_info(threadData, statement->request, 
						 explainInfo, sizeof(explainInfo), explain_ptr, BUFFER_XLARGE);
		}

	UCHAR* plan;
	
	for (int i = 0; i < 2; i++) 
		{
		const UCHAR* explain = explain_ptr;
		if (*explain++ != isc_info_access_path)
			{
			// CVC: deallocate memory!
			if (explain_ptr != explain_buffer) 
				gds__free(explain_ptr);
			return 0;
			}

		int explain_length = *explain++;
		explain_length += (UCHAR) (*explain++) << 8;

		plan = buffer_ptr;

        /* CVC: What if we need to do 2nd pass? Those variables were only initialized
           at the begining of the function hence they had trash the second time. */
        USHORT join_count = 0, level = 0;

		// keep going until we reach the end of the explain info 

		while (explain_length > 0 && buffer_length > 0) 
			{
			if (!getRsbItem(&explain_length, &explain, &buffer_length, &plan,
							  &join_count, &level)) 
				{
				// assume we have run out of room in the buffer, try again with a larger one 

				buffer_ptr = (UCHAR*) gds__alloc(BUFFER_XLARGE);
				buffer_length = BUFFER_XLARGE;
				break;
				}
			}

		if (buffer_ptr == *bufferPtr)
			break;
		}


	if (explain_ptr != explain_buffer)
		gds__free(explain_ptr);

	*bufferPtr = buffer_ptr;

	return plan - *bufferPtr;
}

UCHAR* DStatement::getVariableInfo(dsql_msg *message, const UCHAR *items, const UCHAR *end_describe, UCHAR *info, const UCHAR *end, int first_index)
{
	UCHAR buf[128];
	SLONG sql_type, sql_sub_type, sql_scale, sql_len;

	if (!message || !message->msg_index)
		return info;

	for (const par* param = message->msg_par_ordered; param; 
		param = param->par_ordered)
		{
		if (param->par_index && param->par_index >= first_index) 
			{
			sql_len = param->par_desc.dsc_length;
			sql_sub_type = 0;
			sql_scale = 0;
			switch (param->par_desc.dsc_dtype) 
				{
				case dtype_real:
					sql_type = SQL_FLOAT;
					break;
					
				case dtype_array:
					sql_type = SQL_ARRAY;
					break;

				case dtype_timestamp:
					sql_type = SQL_TIMESTAMP;
					break;
					
				case dtype_sql_date:
					sql_type = SQL_TYPE_DATE;
					break;
					
				case dtype_sql_time:
					sql_type = SQL_TYPE_TIME;
					break;

				case dtype_double:
					sql_type = SQL_DOUBLE;
					sql_scale = param->par_desc.dsc_scale;
					break;

				case dtype_text:
					sql_type = SQL_TEXT;
					sql_sub_type = param->par_desc.dsc_sub_type;
					break;

				case dtype_blob:
					sql_type = SQL_BLOB;
					sql_sub_type = param->par_desc.dsc_sub_type;
					break;

				case dtype_varying:
					sql_type = SQL_VARYING;
					sql_len -= sizeof(USHORT);
					sql_sub_type = param->par_desc.dsc_sub_type;
					break;

				case dtype_short:
				case dtype_long:
				case dtype_int64:
					if (param->par_desc.dsc_dtype == dtype_short)
						sql_type = SQL_SHORT;
					else if (param->par_desc.dsc_dtype == dtype_long)
						sql_type = SQL_LONG;
					else
						sql_type = SQL_INT64;
						
					sql_scale = param->par_desc.dsc_scale;
					
					if (param->par_desc.dsc_sub_type)
						sql_sub_type = param->par_desc.dsc_sub_type;
					break;

				case dtype_quad:
					sql_type = SQL_QUAD;
					sql_scale = param->par_desc.dsc_scale;
					break;

				default:
					ERRD_post(isc_sqlerr, isc_arg_number, -804,
							  isc_arg_gds, isc_dsql_datatype_err, 0);
				}

			if (sql_type && (param->par_desc.dsc_flags & DSC_nullable))
				sql_type++;

			for (const UCHAR* describe = items; describe < end_describe;) 
				{
				USHORT length;
				const TEXT* name;
				const UCHAR* buffer = buf;
				UCHAR item = *describe++;
				switch (item) 
					{
					case isc_info_sql_sqlda_seq:
						length = convert((SLONG) param->par_index, buf);
						break;

					case isc_info_sql_message_seq:
						length = 0;
						break;

					case isc_info_sql_type:
						length = convert((SLONG) sql_type, buf);
						break;

					case isc_info_sql_sub_type:
						length = convert((SLONG) sql_sub_type, buf);
						break;

					case isc_info_sql_scale:
						length = convert((SLONG) sql_scale, buf);
						break;

					case isc_info_sql_length:
						length = convert((SLONG) sql_len, buf);
						break;

					case isc_info_sql_null_ind:
						length = convert((SLONG) (sql_type & 1), buf);
						break;

					case isc_info_sql_field:
						if (name = param->par_name) 
							{
							length = strlen(name);
							buffer = reinterpret_cast<const UCHAR*>(name);
							}
						else
							length = 0;
						break;

					case isc_info_sql_relation:
						if (name = param->par_rel_name) 
							{
							length = strlen(name);
							buffer = reinterpret_cast<const UCHAR*>(name);
							}
						else
							length = 0;
						break;

					case isc_info_sql_owner:
						if (name = param->par_owner_name) 
							{
							length = strlen(name);
							buffer = reinterpret_cast<const UCHAR*>(name);
							}
						else
							length = 0;
						break;

					case isc_info_sql_relation_alias:
						if (name = param->par_rel_alias) 
							{
							length = strlen(name);
							buffer = reinterpret_cast<const UCHAR*>(name);
							}
						else
							length = 0;
						break;


					case isc_info_sql_alias:
						if (name = param->par_alias) 
							{
							length = strlen(name);
							buffer = reinterpret_cast<const UCHAR*>(name);
							}
						else
							length = 0;
						break;

					default:
						buf[0] = item;
						item = isc_info_error;
						length = 1 + convert((SLONG) isc_infunk, buf + 1);
						break;
					}

				if (!(info = put_item(item, length, buffer, info, end)))
					return info;
				}

			if (info + 1 >= end) 
				{
				*info = isc_info_truncated;
				return NULL;
				}
			*info++ = isc_info_sql_describe_end;
			} // if()
		} // for()

	return info;
}

bool DStatement::getRsbItem(int *explain_length_ptr, const UCHAR **explain_ptr, int *plan_length_ptr, UCHAR **plan_ptr, USHORT *parent_join_count, USHORT *level_ptr)
{
	const TEXT* p;
	SSHORT rsb_type, length;
	USHORT join_count, union_count, union_level, union_join_count, save_level;

	int explain_length = *explain_length_ptr;
	const UCHAR* explain = *explain_ptr;
	int plan_length = *plan_length_ptr;
	UCHAR* plan = *plan_ptr;

	explain_length--;

	switch (*explain++) 
		{
		case isc_info_rsb_begin:
			if (!*level_ptr) 
				{
				// put out the PLAN prefix 

				p = "\nPLAN ";
				if ((plan_length -= strlen(p)) < 0)
					return false;
				while (*p)
					*plan++ = *p++;
				}
			(*level_ptr)++;
			break;

		case isc_info_rsb_end:
			if (*level_ptr)
				(*level_ptr)--;
			/* else --*parent_join_count; ??? */
			break;

		case isc_info_rsb_relation:

			/* for the single relation case, initiate
			   the relation with a parenthesis */

			if (!*parent_join_count) 
				{
				if (--plan_length < 0)
					return false;
				*plan++ = '(';
				}

			// if this isn't the first relation, put out a comma 

			if (plan[-1] != '(') 
				{
				plan_length -= 2;
				if (plan_length < 0)
					return false;
				*plan++ = ',';
				*plan++ = ' ';
				}

			// put out the relation name 

			explain_length--;
			explain_length -= (length = (UCHAR) * explain++);
			if ((plan_length -= length) < 0)
				return false;
			while (length--)
				*plan++ = *explain++;
			break;

		case isc_info_rsb_type:
			explain_length--;
			switch (rsb_type = *explain++) 
				{
				/* for stream types which have multiple substreams, print out
				   the stream type and recursively print out the substreams so
				   we will know where to put the parentheses */

				case isc_info_rsb_union:

					// put out all the substreams of the join 

					explain_length--;
					union_count = (USHORT) * explain++ - 1;

					// finish the first union member 

					union_level = *level_ptr;
					union_join_count = 0;
					while (true) 
						{
						if (!getRsbItem(&explain_length, &explain, &plan_length, &plan,
										  &union_join_count, &union_level)) 
							return false;
						if (union_level == *level_ptr)
							break;
						}

					/* for the rest of the members, start the level at 0 so each
					   gets its own "PLAN ... " line */

					while (union_count) 
						{
						union_join_count = 0;
						union_level = 0;
						while (true) 
							{
							if (!getRsbItem(&explain_length, &explain, &plan_length,
											  &plan, &union_join_count, &union_level)) 
								return false;
							if (!union_level)
								break;
							}
						union_count--;
					}
					break;

				case isc_info_rsb_cross:
				case isc_info_rsb_left_cross:
				case isc_info_rsb_merge:

					/* if this join is itself part of a join list,
					   but not the first item, then put out a comma */

					if (*parent_join_count && plan[-1] != '(') {
						plan_length -= 2;
						if (plan_length < 0)
							return false;
						*plan++ = ',';
						*plan++ = ' ';
					}

					// put out the join type 

					if (rsb_type == isc_info_rsb_cross ||
						rsb_type == isc_info_rsb_left_cross) {
						p = "JOIN (";
					}
					else {
						p = "MERGE (";
					}

					if ((plan_length -= strlen(p)) < 0)
						return false;
					while (*p)
						*plan++ = *p++;

					// put out all the substreams of the join 

					explain_length--;
					join_count = (USHORT) * explain++;
			
					while (join_count) 
						{
						if (!getRsbItem(&explain_length, &explain, &plan_length,
										  &plan, &join_count, level_ptr))
							return false;
						// CVC: Here's the additional stop condition. 
						if (!*level_ptr)
							break;
						}


					// put out the final parenthesis for the join 

					if (--plan_length < 0)
						return false;
					else
						*plan++ = ')';

					// this qualifies as a stream, so decrement the join count 

					if (*parent_join_count)
						-- * parent_join_count;
					break;

				case isc_info_rsb_indexed:
				case isc_info_rsb_navigate:
				case isc_info_rsb_sequential:
				case isc_info_rsb_ext_sequential:
				case isc_info_rsb_ext_indexed:
					if (rsb_type == isc_info_rsb_indexed ||
						rsb_type == isc_info_rsb_ext_indexed) 
					{
						p = " INDEX (";
					}
					else if (rsb_type == isc_info_rsb_navigate)
						p = " ORDER ";
					else
						p = " NATURAL";

					if ((plan_length -= strlen(p)) < 0)
						return false;
					while (*p)
						*plan++ = *p++;

					// print out additional index information 

					if (rsb_type == isc_info_rsb_indexed ||
						rsb_type == isc_info_rsb_navigate ||
						rsb_type == isc_info_rsb_ext_indexed) 
						if (!getIndices(&explain_length, &explain, &plan_length, &plan))
							return false;

					if (rsb_type == isc_info_rsb_navigate &&
						*explain == isc_info_rsb_indexed)
						if (!getRsbItem(&explain_length, &explain, &plan_length,
										  &plan, &join_count, level_ptr))
							return false;

					if (rsb_type == isc_info_rsb_indexed ||
						rsb_type == isc_info_rsb_ext_indexed) 
						{
						if (--plan_length < 0)
							return false;
						*plan++ = ')';
						}

					// detect the end of a single relation and put out a final parenthesis 

					if (!*parent_join_count)
						if (--plan_length < 0)
							return false;
						else
							*plan++ = ')';

					// this also qualifies as a stream, so decrement the join count 

					if (*parent_join_count)
						-- * parent_join_count;
					break;

				case isc_info_rsb_sort:

					/* if this sort is on behalf of a union, don't bother to
					   print out the sort, because unions handle the sort on all
					   substreams at once, and a plan maps to each substream
					   in the union, so the sort doesn't really apply to a particular plan */

					if (explain_length > 2 &&
						(explain[0] == isc_info_rsb_begin) &&
						(explain[1] == isc_info_rsb_type) &&
						(explain[2] == isc_info_rsb_union))
					{
						break;
					}

					// if this isn't the first item in the list, put out a comma 

					if (*parent_join_count && plan[-1] != '(') {
						plan_length -= 2;
						if (plan_length < 0)
							return false;
						*plan++ = ',';
						*plan++ = ' ';
					}

					p = "SORT (";

					if ((plan_length -= strlen(p)) < 0)
						return false;
					while (*p)
						*plan++ = *p++;

					/* the rsb_sort should always be followed by a begin...end block,
					   allowing us to include everything inside the sort in parentheses */

					save_level = *level_ptr;
					while (explain_length > 0 && plan_length > 0) {
						if (!getRsbItem(&explain_length, &explain, &plan_length,
										  &plan, parent_join_count, level_ptr))
							return false;
						if (*level_ptr == save_level)
							break;
					}

					if (--plan_length < 0)
						return false;
					*plan++ = ')';
					break;

				default:
					break;
				}
			break;

		default:
			break;
		}

	*explain_length_ptr = explain_length;
	*explain_ptr = explain;
	*plan_length_ptr = plan_length;
	*plan_ptr = plan;

	return true;
}

bool DStatement::getIndices(int *explain_length_ptr, const UCHAR **explain_ptr, int *plan_length_ptr, UCHAR **plan_ptr)
{
	USHORT length;

	int explain_length = *explain_length_ptr;
	const UCHAR* explain = *explain_ptr;
	int plan_length = *plan_length_ptr;
	UCHAR* plan = *plan_ptr;

/* go through the index tree information, just
   extracting the indices used */

	explain_length--;
	switch (*explain++) 
		{
		case isc_info_rsb_and:
		case isc_info_rsb_or:
			if (!getIndices(&explain_length, &explain, &plan_length, &plan))
				return false;
			if (!getIndices(&explain_length, &explain, &plan_length, &plan))
				return false;
			break;

		case isc_info_rsb_dbkey:
			break;

		case isc_info_rsb_index:
			explain_length--;
			length = *explain++;

			// if this isn't the first index, put out a comma 

			if (plan[-1] != '(' && plan[-1] != ' ') {
				plan_length -= 2;
				if (plan_length < 0)
					return false;
				*plan++ = ',';
				*plan++ = ' ';
			}

			// now put out the index name 

			if ((plan_length -= length) < 0)
				return false;
			explain_length -= length;
			while (length--)
				*plan++ = *explain++;
			break;

		default:
			return false;
		}

	*explain_length_ptr = explain_length;
	*explain_ptr = explain;
	*plan_length_ptr = plan_length;
	*plan_ptr = plan;

	return true;
}

void DStatement::instantiateRequest(tdbb* tdsql)
{
	requestInstantiation = statement->getInstantiation();
	dsql_msg *message = statement->sendMessage;
	
	if (message)
		sendMessage = new UCHAR [message->msg_length];
	
	if (message = statement->receiveMessage)
		receiveMessage = new UCHAR [message->msg_length];
}

void DStatement::notYetImplemented()
{
	ERRD_bugcheck ("DStatement::notYetImplemented");
}

ISC_STATUS DStatement::execute(ISC_STATUS* statusVector, Transaction** transactionHandle, 
							   int inBlrLength, const UCHAR* inBlr, 
							   int inMsgType, int inMsgLength, const UCHAR* inMsg, 
							   int outBlrLength, const UCHAR* outBlr, 
							   int outMsgType, int outMsgLength, UCHAR* outMsg)
{
	ISC_STATUS sing_status;
	ThreadData thread (statusVector, attachment);

    try
		{
		sing_status = 0;
		REQ_TYPE requestType = statement->req_type;
		
		if ((SSHORT) inMsgType == -1) 
			requestType = REQ_EMBED_SELECT;

		// Only allow NULL trans_handle if we're starting a transaction 

		if (transaction == NULL && statement->req_type != REQ_START_TRANS)
			ERRD_post(isc_sqlerr, isc_arg_number, -901,
				  	isc_arg_gds, isc_bad_trans_handle, 0);

		/* If the request is a SELECT or blob statement then this is an open.
		   Make sure the cursor is not already open. */

		bool singleton = (statement->req_type == REQ_SELECT && outMsgLength != 0);
		bool needsCursor = false;
		
		switch (requestType)
			{
			case REQ_SELECT:
			case REQ_SELECT_UPD:
			case REQ_EMBED_SELECT:
			case REQ_GET_SEGMENT:
			case REQ_PUT_SEGMENT:
				if (!singleton)
					needsCursor = true;
					
				if (req_flags & REQ_cursor_open)
					ERRD_post(isc_sqlerr, isc_arg_number, -502,
					  		isc_arg_gds, isc_dsql_cursor_open_err, 0);
				break;
			}

		// A select with a non zero output length is a singleton select 
		
		if (requestType != REQ_EMBED_SELECT)
			sing_status = executeRequest (statusVector, transactionHandle, 
										  inBlrLength, inBlr, 
										  inMsgLength, inMsg,
										  outBlrLength, outBlr,
										  outMsgLength, outMsg, 
										  singleton);

		/* If the output message length is zero on a REQ_SELECT then we must
		* be doing an OPEN cursor operation.
		* If we do have an output message length, then we're doing
		* a singleton SELECT.  In that event, we don't add the cursor
		* to the list of open cursors (it's not really open).
		*/

		/***
		if (needsCursor && !cursor)
			cursor = attachment->allocateCursor (this, transaction);
		***/
			/***
			{
			request->req_flags |= REQ_cursor_open |
				((statement->req_type == REQ_EMBED_SELECT) ? REQ_embedded_sql_cursor : 0);

            opn* open_cursor = FB_NEW(*DSQL_permanent_pool) opn;
			request->req_open_cursor = open_cursor;
			open_cursor->opn_request = request;
			open_cursor->opn_transaction = *trans_handle;
			THD_MUTEX_LOCK(&cursors_mutex);
			open_cursor->opn_next = open_cursors;
			open_cursors = open_cursor;
			THD_MUTEX_UNLOCK(&cursors_mutex);
			THREAD_EXIT;
			ISC_STATUS_ARRAY local_status;
			gds__transaction_cleanup(local_status,
								 trans_handle,
								 cleanup_transaction, 0);
			THREAD_ENTER;
			}
			***/

		if (!sing_status)
			return FB_SUCCESS;
		}
	catch (OSRIException& exception)
		{
		return exception.copy (statusVector);
		}

	return sing_status;
}

ISC_STATUS DStatement::executeRequest(ISC_STATUS *statusVector,
									   Transaction** transactionHandle, 
									   int inBlrLength, const UCHAR* inBlr, 
									   int inMsgLength, const UCHAR* inMsg, 
									   int outBlrLength, const UCHAR* outBlr, 
									   int outMsgLength, UCHAR* outMsg, 
									   bool singleton)
{
	ISC_STATUS s;
	ThreadData thread (statusVector, attachment);

	try
		{
		transaction = *transactionHandle;
		int blrLength = statement->getBlrLength();
		const UCHAR *blr = statement->getBlr();

		switch (statement->req_type) 
			{
			case REQ_START_TRANS:
				return jrd8_start_transaction (statusVector, transactionHandle, 1, &attachment, blrLength, blr);

			case REQ_COMMIT:
				return jrd8_commit_transaction (statusVector, transactionHandle);

			case REQ_COMMIT_RETAIN:
				return jrd8_commit_retaining (statusVector, transactionHandle);

			case REQ_ROLLBACK:
				return jrd8_rollback_transaction (statusVector, transactionHandle);

			case REQ_DDL:
				if (s = jrd8_ddl (statusVector, &attachment, transactionHandle, statement->blrGen->getLength(), statement->blrGen->buffer))
					return s;
					
				DDL_execute (statusVector, statement, transaction);
				return FB_SUCCESS;

			/***
			case REQ_GET_SEGMENT:
				execute_blob(	request,
								in_blr_length,
								in_blr,
								in_msg_length,
								in_msg,
								out_blr_length,
								out_blr,
								out_msg_length,
								out_msg);
				return FB_SUCCESS;

			case REQ_PUT_SEGMENT:
				execute_blob(	request,
								in_blr_length,
								in_blr,
								in_msg_length,
								in_msg,
								out_blr_length,
								out_blr,
								out_msg_length,
								out_msg);
				return FB_SUCCESS;
			
			case REQ_EXEC_PROCEDURE:
				if (message = (dsql_msg*) request->req_send) 
					{
					map_in_out(request, message, in_blr_length, in_blr,
							in_msg_length, in_msg);
					in_msg_length = message->msg_length;
					in_msg = message->msg_buffer;
					}
				else 
					{
					in_msg_length = 0;
					in_msg = NULL;
					}
					
				if (out_msg_length && (message = (dsql_msg*) request->req_receive)) 
					{
					if (out_blr_length) 
						parse_blr(out_blr_length, out_blr, out_msg_length, message->msg_parameters);
					use_msg_length = message->msg_length;
					use_msg = message->msg_buffer;
					}
				else 
					{
					use_msg_length = 0;
					use_msg = NULL;
					}

				s = isc_transact_request(tdsql->tsql_status,
										&request->req_dbb->dbb_database_handle,
										&request->req_trans,
										(USHORT) (reinterpret_cast<char*>(request->req_blr) -
												request->req_blr_string->str_data),
										request->req_blr_string->str_data,
										in_msg_length,
										reinterpret_cast<char*>(in_msg),
										use_msg_length,
										reinterpret_cast<char*>(use_msg));
				if (s)
					punt();
				if (out_msg_length && message)
					map_in_out(NULL, message, 0, out_blr, out_msg_length, out_msg);
				return FB_SUCCESS;
			***/
			
			default:
				fb_assert(false);
				// Fall into ... 

			case REQ_EXEC_PROCEDURE:
			case REQ_SELECT:
			case REQ_SELECT_UPD:
			case REQ_INSERT:
			case REQ_DELETE:
			case REQ_UPDATE:
			case REQ_UPDATE_CURSOR:
			case REQ_DELETE_CURSOR:
			case REQ_EMBED_SELECT:
			case REQ_SET_GENERATOR:
			case REQ_SAVEPOINT:
				break;
			}

		// If there is no data required, just start the request 

		dsql_msg *message = statement->sendMessage;
		
		if (message)
			{
			copyData (message, sendMessage, inBlrLength, inBlr, inMsgLength, inMsg, NULL);
			
			if (s = jrd8_start_and_send (statusVector, &statement->request, transactionHandle,
									     message->msg_number, message->msg_length, sendMessage, 
									     requestInstantiation))
				return s;
			}
		else
			if (s = jrd8_start_request (statusVector, &statement->request, transactionHandle, requestInstantiation))
				return s;
			
		singletonFetched = false;
		
		if (outMsgLength && (message = statement->receiveMessage)) 
			{
			/* Insure that the blr for the message is parsed, regardless of
			   whether anything is found by the call to receive. */

			if (s = jrd8_receive (statusVector, &statement->request, 
							      message->msg_number, message->msg_length, receiveMessage, requestInstantiation))
				return s;

			copyData (message, receiveMessage, outBlrLength, outBlr, outMsgLength, NULL, outMsg);

			// if this is a singleton select, make sure there's in fact one record 

#ifdef WORK_PENDING
			if (singleton)
				{
				USHORT counter;

				/* Create a temp message buffer and try two more receives.
				If both succeed then the first is the next record and the
				second is either another record or the end of record message.
				In either case, there's more than one record. */

				UCHAR* message_buffer = new UCHAR[message->msg_length];
				s = 0;
				
				for (counter = 0; counter < 2 && !s; counter++)
					s = isc_receive(local_status,
									&request->req_handle,
									message->msg_number,
									message->msg_length,
									message_buffer,
									0);

				delete[] message_buffer);

				/* two successful receives means more than one record
				a req_sync error on the first pass above means no records
				a non-req_sync error on any of the passes above is an error */

				if (!s)
					{
					tdsql->tsql_status[0] = isc_arg_gds;
					tdsql->tsql_status[1] = isc_sing_select_err;
					tdsql->tsql_status[2] = isc_arg_end;
					return_status = isc_sing_select_err;
					}
				else if (s == isc_req_sync && counter == 1)
					{
					tdsql->tsql_status[0] = isc_arg_gds;
					tdsql->tsql_status[1] = isc_stream_eof;
					tdsql->tsql_status[2] = isc_arg_end;
					return_status = isc_stream_eof;
					}
				else if (s != isc_req_sync)
					punt();
				}
#endif
			}

		/***
		if (!(request->req_dbb->dbb_flags & DBB_v3))
			{
			if (request->req_type == REQ_UPDATE_CURSOR)
				{
				GDS_DSQL_SQL_INFO_CPP(	local_status,
										&request,
										sizeof(sql_records_info),
										sql_records_info,
										sizeof(buffer),
										buffer);
				if (!request->req_updates)
					ERRD_post(isc_sqlerr, isc_arg_number, -913,
							isc_arg_gds, isc_deadlock, isc_arg_gds,
							isc_update_conflict, 0);
				}
			else if (request->req_type == REQ_DELETE_CURSOR)
				{
				GDS_DSQL_SQL_INFO_CPP(	local_status,
										&request,
										sizeof(sql_records_info),
										sql_records_info,
										sizeof(buffer),
										buffer);
				if (!request->req_deletes)
					ERRD_post(isc_sqlerr, isc_arg_number, -913,
							isc_arg_gds, isc_deadlock, isc_arg_gds,
							isc_update_conflict, 0);
				}
			}
		***/

		return statusVector [1];
		}
	catch (OSRIException& exception)
		{
		return exception.copy (statusVector);
		}
}


void DStatement::copyData(dsql_msg* source, UCHAR *msgBuffer, int blrLength, const UCHAR* blr, int msgLength, const UCHAR *inMsg, UCHAR* outMsg)
{
	if (blrLength == 0)
		return;

	//isc_print_blr ((const char*) blr, NULL, NULL, NULL);
	BlrParse parse (blr);
	parse.getVersion();
	int begin = parse.getByte();
	int verb = parse.getByte();
	int msgNumber = parse.getByte();
	int count = parse.getWord();
	int offset = 0;
	JrdMove mover;
    int index, parmcount;
    
    parmcount = count / 2; /* don't include for nulls */
    
    for (index = 1; index <= parmcount; index++)
		{
        par *parameter = source->msg_par_ordered;
        dsc desc, nullDesc;
        int n;

        parse.getBlrDescriptor (&desc);
        int alignment = type_alignments[desc.dsc_dtype];

  	    for (n = 0; parameter && n < count; ++n, parameter = parameter->par_ordered)
			{
            if (parameter->par_index == index)
                {
                dsc parDesc = parameter->par_desc;
				parDesc.dsc_address = msgBuffer + (IPTR) parameter->par_desc.dsc_address;
		
				if (DTYPE_IS_TEXT(parDesc.dsc_dtype))
					parDesc.dsc_sub_type = 0;
					
				if (alignment)
					offset = FB_ALIGN (offset, alignment);
				
				if (inMsg)
					{
					desc.dsc_address = (UCHAR*) inMsg + offset;
					mover.move (&desc, &parDesc);
					}
				else
					{
					desc.dsc_address = outMsg + offset;
					mover.move (&parDesc, &desc);
					}
				
				offset += desc.dsc_length;
	                    
				/* handle the null */
	            
				if (parameter->par_null)
					{
        			parse.getBlrDescriptor (&desc);
					alignment = type_alignments[desc.dsc_dtype];
					parDesc = parameter->par_null->par_desc;
					parDesc.dsc_address = msgBuffer + (IPTR) parDesc.dsc_address;
	                        
					if (alignment)
						offset = FB_ALIGN (offset, alignment);
				
					if (inMsg)
						{
						desc.dsc_address = (UCHAR*) inMsg + offset;
						mover.move (&desc, &parDesc);
						}
					else
						{
						desc.dsc_address = outMsg + offset;
						mover.move (&parDesc, &desc);
						}
	                        
					offset += desc.dsc_length;
					}
	                
				break;
				}
			}
			
        if (n >= count)
	        ERRD_bugcheck ("Parameter index not found.");
		}
    
    /***
	par* dbkey;
	if (request &&
		((dbkey = request->req_parent_dbkey) != NULL) &&
		((parameter = request->req_dbkey) != NULL))
	{
		MOVD_move(&dbkey->par_desc, &parameter->par_desc);
		par* null = parameter->par_null;
		if (null != NULL)
		{
			SSHORT* flag = (SSHORT *) null->par_desc.dsc_address;
			*flag = 0;
		}
	}
	***/
	
	if (inMsg && parent)
		{
		if (statement->req_dbkey && statement->parentDbkey)
			{
			dsc toDesc = statement->req_dbkey->par_desc;
			toDesc.dsc_address = msgBuffer + (IPTR) toDesc.dsc_address;
			dsc fromDesc = statement->parentDbkey->par_desc;
			fromDesc.dsc_address = parent->receiveMessage + (IPTR) fromDesc.dsc_address;
			mover.move (&fromDesc, &toDesc);
			}
			
		if (inMsg && statement->recordVersion && statement->parentRecordVersion)
			{
			dsc toDesc = statement->recordVersion->par_desc;
			toDesc.dsc_address = msgBuffer + (IPTR) toDesc.dsc_address;
			dsc fromDesc = statement->parentRecordVersion->par_desc;
			fromDesc.dsc_address = parent->receiveMessage + (IPTR) fromDesc.dsc_address;
			mover.move (&fromDesc, &toDesc);
			}
		}
		
	/***
	par* rec_version;
	if (request &&
		((rec_version = request->req_parent_rec_version) != NULL) &&
		((parameter = request->req_rec_version) != NULL))
	{
		MOVD_move(&rec_version->par_desc, &parameter->par_desc);
		par* null = parameter->par_null;
		if (null != NULL)
		{
			SSHORT* flag = (SSHORT *) null->par_desc.dsc_address;
			*flag = 0;
		}
	}
	***/

}


ISC_STATUS DStatement::fetch(ISC_STATUS* statusVector, int blrLength, const UCHAR* blr, int msgType, int msgLength, UCHAR* msg)
{
	ThreadData thread (statusVector, attachment);

	if (!statement->req_eof && singletonFetched)
		return 100;
		
	try
		{
		dsql_msg *message = statement->receiveMessage;
		ISC_STATUS s = jrd8_receive (statusVector, 
									&statement->request,
									message->msg_number, 
									message->msg_length, 
									receiveMessage, 
									requestInstantiation);
		
		if (statement->req_eof &&
			  !*(short*)(receiveMessage + (IPTR) statement->req_eof->par_desc.dsc_address))
			return 100;
			
		if (!s)
			{
			singletonFetched = true;
			copyData (message, receiveMessage, blrLength, blr, msgLength, NULL, msg);
			}

		return s;
		}
	catch (OSRIException& exception)
		{
		return exception.copy (statusVector);
		}
}

ISC_STATUS DStatement::freeStatement(ISC_STATUS* statusVector, int options)
{
	ThreadData thread (statusVector, attachment);

	try
		{
		if (statement)
			if (options & DSQL_close)
				closeCursor();

		if (options & DSQL_drop)
			{
			closeStatement();
			delete this;
			}
		}
	catch (OSRIException& exception)
		{
		return exception.copy (statusVector);
		}
		
	return 0;
}

ISC_STATUS DStatement::setCursor(ISC_STATUS* statusVector, const TEXT* cursorName, int cursorType)
{
	try
		{
		JString name = Cursor::getCursorName(cursorName);
		int length = name.length();

		if (length == 0) 
			ERRD_post(isc_sqlerr, isc_arg_number, -502,
					  isc_arg_gds, isc_dsql_decl_err, 0);

		if (cursor)
			{
			if (cursor->name == name)
				return returnSuccess(statusVector);

			ERRD_post(isc_sqlerr, isc_arg_number, -502,
					  isc_arg_gds, isc_dsql_decl_err, 0);
			}
	
		if (attachment->findCursor(name))
			ERRD_post(isc_sqlerr, isc_arg_number, -502,
					  isc_arg_gds, isc_dsql_decl_err, 0);

		cursor = attachment->allocateCursor(this, transaction);
		cursor->name = name;		
		//const dsql_sym* symbol = HSHD_lookup(attach, cursor, length, SYM_cursor, 0);
		}
	catch (OSRIException &exception)
		{
		return exception.copy (statusVector);
		}
	
	return returnSuccess(statusVector);
}

void DStatement::closeCursor(void)
{
}

void DStatement::closeStatement(void)
{
	if (parent)
		{
		parent->removeChild(this);
		parent = NULL;
		}
		
	if (cursor)
		clearCursor();
	
	if (statement)
		{
		/***
		ISC_STATUS statusVector [20];
		if (request)
			{
			jrd8_release_request (statusVector, &request);
			request = NULL;
			}
		***/
		statement->release();
		statement = NULL;
		}
	
	if (sendMessage)
		{
		delete [] sendMessage;
		sendMessage = NULL;
		}
	
	if (receiveMessage)
		{
		delete [] receiveMessage;
		receiveMessage = NULL;
		}
}

// create table yachts (name varchar (30), loa integer);

ISC_STATUS DStatement::executeImmediate(ISC_STATUS* statusVector,
										Transaction** transactionHandle, 
										int sqlLength,  const char* sql, int dialect, 
										int inBlrLength, const UCHAR* inBlr, 
										int inMsgType, int inMsgLength, const UCHAR* inMsg, 
										int outBlrLength, const UCHAR* outBlr, 
										int outMsgType, int outMsgLength, UCHAR* outMsg)
{
	ThreadData thread (statusVector, attachment);

	try
		{
		ISC_STATUS ret = prepare (statusVector, *transactionHandle, sqlLength, sql, dialect, 0, NULL, 0, NULL);
		
		if (ret)
			return ret;
			
		ret = executeRequest (statusVector, transactionHandle, 
								inBlrLength, inBlr, inMsgLength, inMsg, 
								outBlrLength, outBlr, 
								outMsgLength, outMsg, false);
		
		return ret;
		}
	catch (OSRIException &exception)
		{
		return exception.copy (statusVector);
		}
}

ISC_STATUS DStatement::executeDDL(ISC_STATUS* statusVector)
{
	return ISC_STATUS();
}

void DStatement::clearCursor(void)
{
	if (cursor)
		{
		attachment->deleteCursor(cursor);
		cursor = NULL;
		}
}

void DStatement::addChild(DStatement* child)
{
	child->parent = this;
	child->sibling = offspring;
	offspring = child;
}

void DStatement::removeChild(DStatement* child)
{
	for (DStatement **ptr = &offspring; *ptr; ptr = &(*ptr)->sibling)
		if (*ptr == child)
			{
			*ptr = child->sibling;
			break;
			}
}
