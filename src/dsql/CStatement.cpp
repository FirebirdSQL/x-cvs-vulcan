/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		CStatement.h
 *	DESCRIPTION:	Compiled SQL Statement
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
 * December 27, 2003	Created by James A. Starkey
 *
 */

// CStatement.cpp: implementation of the CStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#ifdef MVS
#include <strings.h> // for strcasecmp
#endif
#include "fbdev.h"
#include "common.h"
#include "constants.h"
#include "CStatement.h"
#include "dsql.h"
#include "blr.h"
#include "thd_proto.h"
#include "errd_proto.h"
#include "pass1_proto.h"
#include "gen_proto.h"
#include "sch_proto.h"
#include "jrd_proto.h"
#include "make_proto.h"
#include "iberror.h"
#include "Attachment.h"
#include "Database.h"
#include "Relation.h"
#include "Field.h"
#include "BlrGen.h"
#include "status.h"
#include "ibase.h"
#include "JVector.h"
#include "old_vector.h"
#include "ConfObject.h"
#include "Parameters.h"
#include "SQLParse.h"
#include "OSRIException.h"
#include "alld_proto.h"
#include "hsh_proto.h"
#include "parse_proto.h"
#include "../jrd/intl.h"
#include "ProcManager.h"
#include "Procedure.h"
#include "Connect.h"
#include "PStatement.h"
#include "RSet.h"
#include "tra.h"
#include "Sync.h"
#include "Mutex.h"
#include "Request.h"
#include "fun_proto.h"
#include "align.h"
#include "CharSet.h"
//#include "CharSetContainer.h"
#include "CsConvertArray.h"

#ifdef _WIN32
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif


static Mutex initMutex;
extern dsql_nod* DSQL_parse;

static int initialized; // = DStatement::staticInitialization();

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStatement::CStatement(Attachment *attach)
{
	attachment = attach;					// used for prepare only!
	database = attachment->att_database;
	sibling = NULL;
	offspring = NULL;
	parent = NULL;
	charset = attachment->att_charset;
	sendMessage = NULL;
	receiveMessage = NULL;
	contextNumber = 0;
	scopeLevel = 0;
	loopLevel = 0;
	inOuterJoin = false;
	aliasRelationPrefix = NULL;
	flags = 0;
	procedure = NULL;
	blob = NULL;
	threadData = NULL;				// used only on "prepare"
	req_in_where_clause = 0;		//!< processing "where clause"
	req_in_group_by_clause = 0;		//!< processing "group by clause"
	req_in_having_clause = 0;		//!< processing "having clause"
	req_in_order_by_clause = 0;		//!< processing "order by clause"
	req_in_outer_join = 0;			//!< processing inside outer-join part
	req_error_handlers = 0;			//!< count of active error handlers
	req_client_dialect = 0;			//!< dialect passed into the API call
	req_in_select_list = 0;			//!< now processing "select list"
	cursorNumber = 0;
	ddlNode = NULL;
	syntaxTree = NULL;
	req_eof = NULL;
	req_dbkey = NULL;
	recordVersion = NULL;
	parentRecordVersion = NULL;
	parentDbkey = NULL;
	blrGen = NULL;
	request = NULL;
	//req_blr_string = NULL;
	req_relation = NULL;
	dbb_flags = 0;
	pool = DsqlMemoryPool::createPool();
	oldParameterOrdering = database->configuration->getValue (OldParameterOrdering,OldParameterOrderingValue);
	transaction = NULL;
	execBlockNode = NULL;
	tempCollationName = NULL;
}

CStatement::~CStatement()
{
	ISC_STATUS statusVector [20];
	
	if (request)
		jrd8_release_request (statusVector, &request);
		
	deleteSyntaxTree();
	delete sendMessage;
	delete receiveMessage;
	delete blrGen;
	pool->deletePool (pool);
}

void CStatement::prepare(thread_db* threadStuff, int sqlLength, const TEXT *sqlString, int userDialect)
{
	threadData = threadStuff;
	transaction = threadData->tdbb_transaction;
	
	if (!sqlLength)
		sqlLength = strlen (sqlString);

	while (sqlLength && (sqlString [sqlLength - 1] == ' ' || sqlString [sqlLength - 1] == ';'))
		--sqlLength;

	sql.setString (sqlString, sqlLength);

	// Figure out which parser version to use 
	/* Since the API to dsql8_prepare is public and can not be changed, there needs to
	 * be a way to send the parser version to DSQL so that the parser can compare the keyword
	 * version to the parser version.  To accomplish this, the parser version is combined with
	 * the client dialect and sent across that way.  In dsql8_prepare_statement, the parser version
	 * and client dialect are separated and passed on to their final desintations.  The information
	 * is combined as follows:
	 *     Dialect * 10 + parser_version
	 *
	 * and is extracted in dsql8_prepare_statement as follows:
	 *      parser_version = ((dialect *10)+parser_version)%10
	 *      client_dialect = ((dialect *10)+parser_version)/10
	 *
	 * For example, parser_version = 1 and client dialect = 1
	 *
	 *  combined = (1 * 10) + 1 == 11
	 *
	 *  parser = (combined) %10 == 1
	 *  dialect = (combined) / 19 == 1
	 *
	 * If the parser version is not part of the dialect, then assume that the
	 * connection being made is a local classic connection.
	 */

	dialect = userDialect;
	int parserVersion = 2;

	if (dialect / 10)
		{
		parserVersion = dialect % 10;
		dialect /= 10;
		}

    req_client_dialect = dialect;

	if (dialect > SQL_DIALECT_CURRENT)
		ERRD_post(isc_sqlerr, isc_arg_number, (SLONG) - 901,
				  isc_arg_gds, isc_wish_list, 0);

	// Parse the SQL statement.  If it croaks, return 

	SQLParse parse (dialect, database->getSqlDialect(), parserVersion);
	dsql_nod *parseTree = parse.parse (threadData, sqlLength, sql, charset);
	
	if (!parseTree)
		ERRD_post(isc_sqlerr, isc_arg_number, -104,
				  isc_arg_gds, isc_command_end_err, 0);

	// allocate the send and receive messages 

	sendMessage = new dsql_msg;
	receiveMessage = new dsql_msg;
	receiveMessage->msg_number = 1;

#ifdef SCROLLABLE_CURSORS
	if (request->req_dbb->dbb_base_level >= 5) {
		/* allocate a message in which to send scrolling information
		   outside of the normal send/receive protocol */

		request->req_async = message = FB_NEW(*tdsql->tsql_default) dsql_msg;
		message->msg_number = 2;
	}
#endif

	req_type = REQ_SELECT;
	//req_flags &= ~(REQ_cursor_open | REQ_embedded_sql_cursor);

	/*
	 * No work is done during pass1 for set transaction - like
	 * checking for valid table names.  This is because that will
	 * require a valid transaction handle.
	 * Error will be caught at execute time.
	 */

	dsql_nod* node = PASS1_statement(this, parseTree, false);

	if (!node)
		return;

	// stop here for requests not requiring code generation 

	switch (req_type)
		{
		case REQ_DDL:
			ddlNode = node;
			syntaxTree = parse.takeSyntaxTree();
			//if (parse.ambiguousStatement && database->getSqlDialect() != userDialect)
			if (parse.ambiguousStatement && database->getSqlDialect() != req_client_dialect)
				ERRD_post(isc_sqlerr, isc_arg_number, (SLONG) - 817,
						isc_arg_gds, isc_ddl_not_allowed_by_db_sql_dial,
						isc_arg_number,
						database->getSqlDialect(), 0);
			break;
		
		case REQ_COMMIT:
		case REQ_COMMIT_RETAIN:
		case REQ_ROLLBACK:
			return;
		
		case REQ_GET_SEGMENT:
		case REQ_PUT_SEGMENT:
			GEN_port(this, blob->blb_open_in_msg);
			GEN_port(this, blob->blb_open_out_msg);
			GEN_port(this, blob->blb_segment_msg);
			return;
		}

	// Generate BLR, DDL or TPB for request 

	if (blrGen)
		delete blrGen;

	blrGen = new BlrGen();

	/* Start transactions takes parameters via a parameter block.
	   The request blr string is used for that. */

	if (req_type == REQ_START_TRANS) 
		{
		GEN_start_transaction(this, node);
		return;
		}

	if (userDialect > SQL_DIALECT_V5)
		flags |= REQ_blr_version5;
	else
		flags |= REQ_blr_version4;

	try
		{
		GEN_request(this, node);
		}
	catch (...)
		{
		if (req_relation)
			req_relation->purgeTemporaryFields();
		throw;
		}
	
	if (req_relation)
		req_relation->purgeTemporaryFields();
			
	int length = blrGen->getLength();
	
	// stop here for ddl requests 

	if (req_type == REQ_DDL)
		return;

	// have the access method compile the request 

#ifdef DSQL_DEBUG
	if (DSQL_debug & 64) 
		{
		dsql_trace("Resulting BLR code for DSQL:");
		gds__print_blr(blrGen->buffer, gds__trace_printer, 0, 0);
		}
#endif

	/***
	if (req_type == REQ_EXEC_PROCEDURE)
		return;
	
	// check for warnings 

	if (tdsql->tsql_status[2] == isc_arg_warning) 
		{
		// save a status vector 
		MOVE_FASTER(tdsql->tsql_status, local_status,
					sizeof(ISC_STATUS) * ISC_STATUS_LENGTH);
		}
	***/
	
	ISC_STATUS localStatus [20];
	
	//if (req_type == REQ_UPDATE_CURSOR)
		//blrGen->print();
	
	if (jrd8_compile_request (localStatus, &attachment, &request, 
							  blrGen->getLength(), blrGen->buffer))
		throw OSRIException (localStatus);
		
	/***
	// restore warnings (if there are any) 

	if (local_status[2] == isc_arg_warning)
		{
		int indx, len, warning;

		// find end of a status vector 
		PARSE_STATUS(tdsql->tsql_status, indx, warning);
		if (indx)
			--indx;

		// calculate length of saved warnings 
		PARSE_STATUS(local_status, len, warning);
		len -= 2;

		if ((len + indx - 1) < ISC_STATUS_LENGTH)
			MOVE_FASTER(&local_status[2], &tdsql->tsql_status[indx],
						sizeof(ISC_STATUS) * len);
	}

	delete req_blr_string;
	req_blr_string = NULL;

	if (status)
		punt();
	***/
	
	transaction = NULL;
}


dsql_rel* CStatement::findRelation(const char *relationName)
{
	Relation *jrdRelation = (transaction) ? transaction->findRelation(threadData, relationName) :
											database->findRelation (threadData, relationName);

	if (!jrdRelation)
		{
		if (req_relation && req_relation->rel_name == relationName)
			return req_relation;
		return NULL;
		}

#ifdef SHARED_CACHE
	Sync sync (&jrdRelation->syncObject, "CStatement::findRelation");
	sync.lock (Shared);
#endif

	if (jrdRelation->dsqlRelation)
		return jrdRelation->dsqlRelation;

#ifdef SHARED_CACHE
	sync.unlock();
	sync.lock (Exclusive);
#endif
	
	if (jrdRelation->dsqlRelation)
		return jrdRelation->dsqlRelation;

	if (!(jrdRelation->rel_flags & REL_scanned))
		jrdRelation->scan(threadData);

    dsql_rel *relation = jrdRelation->dsqlRelation = new dsql_rel;
	relation->rel_name = jrdRelation->rel_name;
	relation->rel_id = jrdRelation->rel_id;
	relation->rel_owner = jrdRelation->rel_owner_name;
	relation->rel_dbkey_length = jrdRelation->rel_dbkey_length;
	relation->rel_fields = NULL;
	relation->jrdRelation = jrdRelation;
	dsql_fld **ptr = &relation->rel_fields;

	if (!(jrdRelation->rel_flags & REL_has_type_info))
		jrdRelation->getTypeInformation (threadData);

        if (jrdRelation->rel_view_rse)
              relation->rel_flags |= REL_view;

	//printf ("CStatement::findRelation %s\n", relationName);

	//FOR_VEC (Field*, jrdField, jrdRelation->rel_fields)
	for (int n = 0; n < jrdRelation->rel_fields.size(); ++n)
		{
		Field *jrdField = jrdRelation->rel_fields[n];
		/***
		if (jrdField)
			printf ("  %s %x\n", jrdField->fld_name, jrdField->dsqlField);
		***/
		if (jrdField && !jrdField->dsqlField)
			{
			dsql_fld *field = *ptr = new dsql_fld;
			field->field = jrdField;
			field->fld_dtype = jrdField->fld_dtype;		
			field->fld_scale = jrdField->fld_scale;		
			field->fld_length = jrdField->fld_precision;		
			field->fld_sub_type = jrdField->fld_sub_type;		
			field->fld_ttype = jrdField->fld_sub_type;		
			field->fld_dimensions = jrdField->fld_dimensions;		
			field->fld_id = jrdField->fld_id;		
			
			if (!jrdField->fld_not_null)
				field->fld_flags |= FLD_nullable;
				
            if (field->fld_dtype <= dtype_any_text)  
				{
                field->fld_character_set_id = INTL_TYPE_TO_CS(field->fld_sub_type);
                field->fld_collation_id = INTL_TYPE_TO_COL(field->fld_sub_type);
				}
			
			field->fld_name = jrdField->fld_name;
			jrdField->dsqlField = field;
			ptr = &field->fld_next;
			}
		}
	//END_FOR

	relation->orderFields();
	
	return relation;
}

Procedure* CStatement::findProcedure(const TEXT *name)
{
	ProcManager *procManager = database->procManager;
	Procedure *procedure = procManager->findProcedure (threadData, name, FALSE);
	return procedure;
}

CharSetContainer* CStatement::findCharset(const TEXT *name)
{
	return database->findCharset (threadData, name);
}

CharSetContainer* CStatement::findCollation(const char *name)
{
	return database->findCollation (threadData, name);
}

dsql_rel* CStatement::findViewRelation(const TEXT *viewName, const TEXT *relationOrAlias, int level)
{
/*  Starting with a view name and a relation name or context name, recurse through
    view definitions trying to find a reference to that table or context name.  */

	if (strlen (viewName) > MAX_SQL_IDENTIFIER_SIZE)
		return NULL;

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT"
		"		vrel.RDB$CONTEXT_NAME,"
		"		vrel.RDB$RELATION_NAME"
		"	FROM RDB$VIEW_RELATIONS vrel"
		"	WHERE vrel.RDB$VIEW_NAME = ?");

	statement->setString(1, viewName);
	RSet resultSet = statement->executeQuery();
	
	while (resultSet->next())
		{
		JString contextName = stripString (resultSet->getString(1));
		JString relationName = stripString (resultSet->getString(2));

		if (!strcmp(relationName, relationOrAlias) ||
			!strcmp(contextName, relationOrAlias))
			{
			return getRelation (relationName);
			}

		dsql_rel *relation = findViewRelation(relationName,
		                           relationOrAlias, level + 1);
		if (relation) 
			return relation;
		}

	return NULL;
}

UserFunction* CStatement::findFunction(const TEXT *functionName)
{
	UserFunction* function = FUN_lookup_function (threadData, functionName, false);

	return function;
}

void CStatement::dropRelation(const TEXT *relationName)
{
	//notYetImplemented();
}

void CStatement::dropProcedure(const TEXT *procedureName)
{
	ProcManager *procManager = database->procManager;
	//procManager->dropProcedure (procedureName);
}

void CStatement::dropFunction(const TEXT *functionName)
{
	//notYetImplemented();
}

int CStatement::getType(const TEXT *typeName, const TEXT *fieldName, short *value)
{
	if ((strlen (typeName) > MAX_SQL_IDENTIFIER_SIZE) ||
		(strlen (fieldName) > MAX_SQL_IDENTIFIER_SIZE))
		return 0;

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT typ.RDB$TYPE "
		"	FROM RDB$TYPES typ "
		"	WHERE typ.RDB$TYPE_NAME = ? AND typ.RDB$FIELD_NAME = ?");

	statement->setString (1, typeName);
	statement->setString (2, fieldName);

	RSet resultSet = statement->executeQuery();
	
	if (!resultSet->next())
		return 0;
		
	int type = resultSet->getInt (1);

	if (resultSet->wasNull())
		return 0;

	*value = type;
	
	return type;
}

int CStatement::getCharsetBytesPerCharacter(int charsetId)
{

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT chs.RDB$BYTES_PER_CHARACTER "		//1
		"	FROM RDB$CHARACTER_SETS chs "
		"	WHERE csh.RDB$CHARACTER_SET_ID = ? ");
		
	statement->setInt(1, charsetId);
	RSet resultSet = statement->executeQuery();
	
	if (!resultSet->next())
		return 1;
		
	int bpc = resultSet->getInt(1);
	if (resultSet->wasNull())
		bpc = 1;

	return bpc;
}

dsql_str* CStatement::getDefaultCharset()
{
	if (database->dbb_flags & DBB_no_charset)
		return NULL;

	if (database->defaultCharSet)
		{
		CharSetContainer *charSetContainer = database->defaultCharSet;
		CharSet charSet = database->defaultCharSet->getCharSet();
		const char * name = charSet.getName();
		return MAKE_string (threadData, name, strlen(name));
		}

	Connect connection = attachment->getUserConnection(transaction);
	//Connect connection = database->getSystemConnection();
	PStatement statement = connection->prepareStatement (
		"SELECT dbb.RDB$CHARACTER_SET_NAME "		
		"	FROM RDB$DATABASE dbb ");
		
	RSet resultSet = statement->executeQuery();
	
	if (!resultSet->next())
		{
		database->dbb_flags |= DBB_no_charset;
		return NULL;
		}

	const char *name = resultSet->getString(1);
	if (resultSet->wasNull())
		return NULL;

	JString strippedName = stripString (name);
	database->defaultCharSet = findCharset(strippedName);
	return MAKE_string (threadData, strippedName, strippedName.length());

}

void CStatement::blrBegin(UCHAR verb)
{
	/***
	if (verb)
		appendUCHAR(verb);

	req_base_offset = req_blr - reinterpret_cast<BLOB_PTR*>(req_blr_string->str_data);

	// put in a place marker for the size of the blr, since it is unknown
	appendShort(0);
	***/
	blrGen->blrBeginSubstring(verb);
	appendUCHAR((flags & REQ_blr_version4) ? blr_version4 : blr_version5);
}

void CStatement::blrEnd()
{
	appendUCHAR(blr_eoc);
	// go back and stuff in the proper length

	/***
	char* blr_base = req_blr_string->str_data + req_base_offset;
	const USHORT length   = (USHORT) (reinterpret_cast<char*>(req_blr) - blr_base) - 2;
	*blr_base++ = (UCHAR) length;
	*blr_base = (UCHAR) (length >> 8);
	***/
	blrGen->blrEndSubstring();
}


bool CStatement::getDomainDefault(const TEXT *domainName, TEXT *buffer, int bufferLength)
{

	if (strlen (domainName) > MAX_SQL_IDENTIFIER_SIZE) 
		return false;

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT fld.RDB$DEFAULT_VALUE "
		"	FROM RDB$FIELDS fld "
		"	WHERE fld.RDB$FIELD_NAME = ?");

	statement->setString (1, domainName);

	RSet resultSet = statement->executeQuery();

	if (!resultSet->next())
		return false;

	Blob *blob = resultSet->getBlob(1);
	
	if (resultSet->wasNull())
		{
		buffer[0] = blr_version4;
		buffer[1] = blr_eoc;
		}
	else
		{
		if (blob->length() > bufferLength)
			ERRD_bugcheck ("default value too long CStatement::getDomainDefault");

		blob->getBytes(0, bufferLength, buffer);
		blob->release();
		if ((buffer[0] != blr_version4 && buffer[0] != blr_version5) ||
			buffer[1] == blr_literal)
			ERRD_bugcheck ("badly formed default value CStatement::getDomainDefault");
		}

	return true;
}


void CStatement::genUnnamedTriggerBeginning(bool onUpdateTrigger, 
											const TEXT *primaryRelationName, 
											dsql_nod *primaryColumns, 
											const TEXT *foreignRelationName, 
											dsql_nod *foreignColumns)
{
	// no trigger name. It is generated by the engine
	appendDynString(isc_dyn_def_trigger, "", 0);

	appendNumber(isc_dyn_trg_type,
			   (onUpdateTrigger) ? POST_MODIFY_TRIGGER : POST_ERASE_TRIGGER);

	appendUCHAR(isc_dyn_sql_object);
	appendNumber(isc_dyn_trg_sequence, 1);
	appendNumber(isc_dyn_trg_inactive, 0);
	appendDynString(isc_dyn_rel_name, primaryRelationName);

	// the trigger blr
	blrBegin(isc_dyn_trg_blr);

	/* for ON UPDATE TRIGGER only: generate the trigger firing condition:
	   if prim_key.old_value != prim_key.new value.
	   Note that the key could consist of multiple columns */

	if (onUpdateTrigger) 
		{
		genFiringCondition(primaryColumns);
		appendUCHARs(blr_begin, 2);
		}

	appendUCHAR(blr_for);
	appendUCHAR(blr_rse);

	// the context for the prim. key relation
	appendUCHAR(1);
	appendUCHAR(blr_relation);
	appendBlrString(foreignRelationName);
	// the context for the foreign key relation
	appendUCHAR(2);

	// generate the blr for: foreign_key == primary_key
	genMatchingKey(foreignColumns, primaryColumns);

	appendUCHAR(blr_modify);
	appendUCHAR(2);
	appendUCHAR(2);
	appendUCHAR(blr_begin);
}

bool CStatement::getDomain(const TEXT *domainName, dsql_fld *field)
{
	if (strlen(domainName) > MAX_SQL_IDENTIFIER_SIZE)
		return false;

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT"
		"  fld.RDB$FIELD_LENGTH,"		//1
		"  fld.RDB$FIELD_SCALE,"		//2
		"  fld.RDB$FIELD_SUB_TYPE,"		//3
		"  fld.RDB$CHARACTER_SET_ID,"	//4
		"  fld.RDB$COLLATION_ID,"		//5
		"  fld.RDB$CHARACTER_LENGTH,"	//6
		"  fld.RDB$COMPUTED_BLR,"		//7
		"  fld.RDB$FIELD_TYPE, "		//8
		"  fld.RDB$SEGMENT_LENGTH "		//9
		"FROM"
		"  RDB$FIELDS fld "
		"WHERE"
		"  fld.RDB$FIELD_NAME = ? ");
		
	statement->setString(1, domainName);
	RSet resultSet = statement->executeQuery();
	
	if (!resultSet->next())
		return false;
		
	field->fld_length = resultSet->getInt(1);
    field->fld_scale = resultSet->getInt(2);
    field->fld_sub_type = resultSet->getInt(3);

	field->fld_character_set_id = resultSet->getInt(4);
	if (resultSet->wasNull())
		field->fld_character_set_id = 0;

	field->fld_collation_id = resultSet->getInt(5);
 	if (resultSet->wasNull())
		field->fld_collation_id = 0;

	field->fld_character_length = resultSet->getInt(6);        
	if (resultSet->wasNull())
		field->fld_character_length = 0;

	Blob *blob = resultSet->getBlob(7);
	if (!resultSet->wasNull())
        field->fld_flags |= FLD_computed;
    
    blob->release();
    convertDType (field, resultSet->getInt(8));

    if (resultSet->getInt(8) == blr_blob) 
        field->fld_seg_length = resultSet->getInt(9);
        
    return true;
}

bool CStatement::getColumnDefault(const TEXT *relationName, const TEXT *columnName, TEXT *buffer, int bufferLength)
{
	if ((strlen (relationName) > MAX_SQL_IDENTIFIER_SIZE) ||
		(strlen (columnName) > MAX_SQL_IDENTIFIER_SIZE))
			return false;

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT "
		    "fld.RDB$DEFAULT_VALUE, "
			"rfl.RDB$DEFAULT_VALUE "
		"FROM RDB$RELATION_FIELDS rfl "
		    "INNER JOIN RDB$FIELDS fld "
			"ON (rfl.RDB$FIELD_SOURCE = fld.RDB$FIELD_NAME) "
		"WHERE RFL.RDB$RELATION_NAME = ? "
		   "AND RFL.RDB$FIELD_NAME = ?");

	statement->setString(1, relationName);
	statement->setString(2, columnName);
	RSet resultSet = statement->executeQuery();

	bool noDomainDefault;
	bool noColumnDefault;
	Blob *domainDefault;
	Blob *columnDefault;

	if (!resultSet->next())
		return false;

	domainDefault = resultSet->getBlob(1);
	noDomainDefault = resultSet->wasNull();
	columnDefault = resultSet->getBlob(2);
	noColumnDefault = resultSet->wasNull();

	if (noColumnDefault && noDomainDefault)
		{
		columnDefault->release();
		domainDefault->release();
		return false;
		}

	if (noColumnDefault)
		{
		columnDefault->release();
		columnDefault = domainDefault;
		}
	else 
		domainDefault->release();

	if (columnDefault->length() > bufferLength)
		ERRD_bugcheck ("default value too long CStatement::getColumnDefault");

	columnDefault->getBytes(0, bufferLength, buffer);
	columnDefault->release();
	return true;
}

dsql_str* CStatement::getTriggerRelation(const TEXT *triggerName, USHORT *triggerType)
{
	if (strlen (triggerName) > MAX_SQL_IDENTIFIER_SIZE)
		return NULL;

	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT "
		"		trg.RDB$RELATION_NAME,"
		"		trg.RDB$TRIGGER_TYPE"
		"	FROM RDB$TRIGGERS trg"
		"	WHERE trg.RDB$TRIGGER_NAME = ?");

	statement->setString(1, triggerName);
	RSet resultSet = statement->executeQuery();


	for (int n = 0; resultSet->next(); ++n)
		{
		*triggerType = resultSet->getShort(2);
		const char *relationName =resultSet->getString(1);
		JString strippedName = stripString (relationName);
		return MAKE_string(threadData, strippedName, strlen(strippedName));
		}
return NULL;
}

dsql_nod* CStatement::getPrimaryKey(const TEXT *relationName)
{
	dsql_rel *relation = findRelation (relationName);
	
	if (relation)
		{
		Field *fields [MAX_INDEX_SEGMENTS];
		if (relation->jrdRelation == NULL) return NULL;
		int count = relation->jrdRelation->getPrimaryKey (threadData, MAX_INDEX_SEGMENTS, fields);
		dsql_nod *list = MAKE_node(threadData, nod_list, count);
		
		for (int n = 0; n < count; ++n)
			{
			dsql_str* field_name = MAKE_cstring(threadData, fields [n]->fld_name);
			dsql_nod* field_node = MAKE_node(threadData, nod_field_name, e_fln_count);
			field_node->nod_arg[e_fln_name] = (DSQL_NOD) field_name;
			list->nod_arg[n] = field_node;
			}
			
		return list;
		}

	/*
	 *  The following piece of historic code is the first sql introduced into the
	 *  Firebird engine.  It was originally coded by Jim Starkey on June 9, 2004.
	 *  As usual, nobody liked his coding.  The code below, chosen as the best
	 *  among a set of many alternatives, was submitted by Arno Brinkman on June 10,
	 *  2004.  Further complaints should be addressed to Mr. Brinkman at
	 *  firebird@abvisie.nl.
	 *
	 * NB, in internal sql, unlike GDML, input parameters must not exceed their anticipated
	 * anticipated size.  A test here will save tears later.  Returning null will result
	 * in a perfectly reasonable error.
	 */

	if (strlen (relationName) > MAX_SQL_IDENTIFIER_SIZE)
		return NULL;
	 		
	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT"
		"  ixs.RDB$FIELD_NAME,"
		"  i.RDB$SEGMENT_COUNT "
		"FROM"
		"  RDB$RELATION_CONSTRAINTS rc"
		"  JOIN RDB$INDEX_SEGMENTS ixs ON"
		"    (ixs.RDB$INDEX_NAME = rc.RDB$INDEX_NAME)"
		"  JOIN RDB$INDICES i ON (i.RDB$INDEX_NAME = ixs.RDB$INDEX_NAME)"
		"WHERE"
		"  rc.RDB$RELATION_NAME = ? AND"
		"  rc.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY'"
		"ORDER BY"
		"  ixs.RDB$FIELD_POSITION");
		
	statement->setString(1, relationName);
	RSet resultSet = statement->executeQuery();
	dsql_nod *list= NULL;
	
	for (int n = 0; resultSet->next(); ++n)
		{
		if (!list)
			list = MAKE_node(threadData, nod_list, resultSet->getInt(2));
		const char *fieldName = resultSet->getString(1);
		JString strippedName = stripString(fieldName);
		dsql_str* field_name = MAKE_string(threadData, strippedName, strlen(strippedName));
		dsql_nod* field_node = MAKE_node(threadData, nod_field_name, e_fln_count);
		field_node->nod_arg[e_fln_name] = (DSQL_NOD) field_name;
		list->nod_arg[n] = field_node;
		}

	return list;
}

void CStatement::notYetImplemented()
{
	ERRD_bugcheck ("CStatement::notYetImplemented");
}

void CStatement::genFiringCondition(dsql_nod *prim_columns)
{
	appendUCHAR(blr_if);

	if (prim_columns->nod_count > 1)
		appendUCHAR(blr_or);

	USHORT num_fields = 0;
	const dsql_nod* const* prim_key_flds = prim_columns->nod_arg;

	do {
		appendUCHAR(blr_neq);

		const dsql_str* prim_key_fld_name_str = (dsql_str*) (*prim_key_flds)->nod_arg[1];

		appendUCHAR(blr_field);
		appendUCHAR(0);
		appendBlrString(prim_key_fld_name_str->str_data);
		appendUCHAR(blr_field);
		appendUCHAR(1);
		appendBlrString(prim_key_fld_name_str->str_data);

		num_fields++;

		if (prim_columns->nod_count - num_fields >= 2)
			appendUCHAR(blr_or);

		prim_key_flds++;

	} while (num_fields < prim_columns->nod_count);
}

void CStatement::genMatchingKey(const dsql_nod *for_columns, const dsql_nod *prim_columns)
{
	fb_assert(prim_columns->nod_count == for_columns->nod_count);
	fb_assert(prim_columns->nod_count != 0);

	appendUCHAR(blr_boolean);
	if (prim_columns->nod_count > 1) {
		appendUCHAR(blr_and);
	}

	USHORT num_fields = 0;
	const dsql_nod* const* for_key_flds = for_columns->nod_arg;
	const dsql_nod* const* prim_key_flds = prim_columns->nod_arg;

	do {
		appendUCHAR(blr_eql);

		const dsql_str* for_key_fld_name_str = (dsql_str*) (*for_key_flds)->nod_arg[1];
		const dsql_str* prim_key_fld_name_str = (dsql_str*) (*prim_key_flds)->nod_arg[1];

		appendUCHAR(blr_field);
		appendUCHAR(2);
		appendBlrString(for_key_fld_name_str->str_data);
		appendUCHAR(blr_field);
		appendUCHAR(0);
		appendBlrString(prim_key_fld_name_str->str_data);

		num_fields++;

		if (prim_columns->nod_count - num_fields >= 2) 
			appendUCHAR(blr_and);

		for_key_flds++;
		prim_key_flds++;

	} while (num_fields < for_columns->nod_count);

	appendUCHAR(blr_end);
}


par* CStatement::makeParameter(dsql_msg* message, bool sqldaFlag, bool nullFlag, int sqldaIndex)
{
	if (sqldaFlag && sqldaIndex && (sqldaIndex <= message->msg_index) && !OldParameterOrdering) 
		// This parameter possibly already here. Look for it
		for (par* temp = message->msg_parameters; temp; temp = temp->par_next) 
			if (temp->par_index == sqldaIndex)
				return temp;

	//TSQL tdsql = GET_THREAD_DATA;

	par* parameter = FB_NEW(*threadData->tsql_default) par;
	parameter->par_message = message;
	parameter->par_next = message->msg_parameters;
	
	if (parameter->par_next != 0)
		parameter->par_next->par_ordered = parameter;
	else
		message->msg_par_ordered = parameter;
		
	message->msg_parameters = parameter;
	parameter->par_parameter = message->msg_parameter++;
	parameter->par_rel_name = NULL;
	parameter->par_owner_name = NULL;

	// If the parameter is used declared, set SQLDA index 

	if (sqldaFlag) 
		{
		if (sqldaIndex && !oldParameterOrdering) 
			{
			parameter->par_index = sqldaIndex;
			if (message->msg_index < sqldaIndex)
				message->msg_index = sqldaIndex;
			}
		else 
			parameter->par_index = ++message->msg_index;
		}
		
	// If a null handing has been requested, set up a null flag 

	if (nullFlag) 
		{
		par* null = makeParameter (message, false, false, 0);
		parameter->par_null = null;
		null->par_desc.dsc_dtype = dtype_short;
		null->par_desc.dsc_scale = 0;
		null->par_desc.dsc_sub_type = 0;
		null->par_desc.dsc_length = sizeof(SSHORT);
		}

	return parameter;
}

const UCHAR* CStatement::getBlr(void)
{
	if (blrGen)
		return blrGen->buffer;
	
	return NULL;
}

int CStatement::getBlrLength(void)
{
	if (blrGen)
		return blrGen->getLength();
	
	return 0;
}

void CStatement::staticInitialization(void)
{
	if (initialized)
		return;
	
	Sync sync (&initMutex, "CStatement::staticInitialization");
	sync.lock(Exclusive);
	
	if (initialized)
		return;
	
#ifdef ANY_THREADING_XXX
	if (!mutex_inited) {
		mutex_inited = true;
		THD_MUTEX_INIT(&databases_mutex);
		THD_MUTEX_INIT(&cursors_mutex);
	}
#endif

	ALLD_init();
	HSHD_init();

#ifdef DSQL_DEBUG
	//DSQL_debug = Config::getTraceDSQL();
#endif

	LEX_dsql_init();

	//gds__register_cleanup(cleanup, 0);
	
	initialized = true;
}

int CStatement::getInstantiation(void)
{
	return 0;
}

void CStatement::releaseInstantiation(int instantiation)
{
}

void CStatement::deleteSyntaxTree(void)
{
	ddlNode = NULL;
	
	for (dsql_nod *node; node = syntaxTree;)
		{
		syntaxTree = node->nod_next;
		delete node;
		}
}

dsql_rel* CStatement::getRelation(const TEXT* relationName)
{
	dsql_rel *relation = findRelation (relationName);
	
	if (!relation)
		ERRD_post(isc_sqlerr, isc_arg_number, (SLONG) - 204, 
				  isc_arg_gds, isc_dsql_relation_err, 
				  isc_arg_gds, isc_random, isc_arg_string, relationName, 
				  //isc_arg_gds,  isc_random, isc_arg_string, linecol, 
				  isc_arg_end);
	
	return relation;
}

void CStatement::getRequestInfo(thread_db* threadData, int instantiation, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	Request *instance = request->getInstantiatedRequest(instantiation);
	request->getRequestInfo(threadData, itemsLength, items, bufferLength, buffer);
}

void CStatement::convertDType (dsql_fld* field,  int blrType)
{
/*	Convert from the blr_<type> stored in system metadata
 *	to the internal dtype_* descriptor.  Also set field
 *	length.
 */

	switch (blrType)
		{
		case blr_text:
			field->fld_dtype = dtype_text;
			break;

		case blr_varying:
			field->fld_dtype = dtype_varying;
			field->fld_length += sizeof(USHORT);
			break;

		case blr_blob:
			field->fld_dtype = dtype_blob;
			field->fld_length = type_lengths[field->fld_dtype];
			break;

		default:		
			field->fld_dtype = gds_cvt_blr_dtype[blrType];
			field->fld_length = type_lengths[field->fld_dtype];

		if (field->fld_dtype == dtype_unknown)
			ERRD_bugcheck ("unknown field type in CStatement::convertDType");

		}
}

JString CStatement::stripString (const TEXT *string)
{
	const char *p = string;

	while (*p )
		p++;

	while (p > string && p[-1] == ' ')
		--p;

	return JString (string, p - string);
}


int CStatement::findColumn(const char *columnName)
{
	if (!receiveMessage)
		return -1;
	
	for (par *parameter = receiveMessage->msg_par_ordered; parameter; parameter = parameter->par_ordered)
		if (parameter->par_name && strcasecmp(parameter->par_name, columnName) == 0)
			return parameter->par_index;
	
	return -1;
}

bool CStatement::existsException (const TEXT *exceptionName)
{
	if (strlen (exceptionName) > MAX_SQL_IDENTIFIER_SIZE)
		return false;
	 		
	Connect connection = attachment->getUserConnection(transaction);
	PStatement statement = connection->prepareStatement (
		"SELECT"
		"  e.RDB$EXCEPTION_NAME "
		"FROM"
		"  RDB$EXCEPTIONS e "
		"WHERE"
		"  e.RDB$EXCEPTION_NAME = ? ");
		
	statement->setString(1, exceptionName);
	RSet resultSet = statement->executeQuery();
	
	return resultSet->next();
}
