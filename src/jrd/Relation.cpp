/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Relation.cpp
 *	DESCRIPTION:	Relation (should be table) class
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

// Relation.cpp: implementation of the Relation class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "jrd.h"
#include "Relation.h"
#include "all.h"
#include "met_proto.h"
#include "thd_proto.h"
#include "../dsql/dsql_rel.h"
#include "iberror.h"
#include "Field.h"
#include "err_proto.h"
#include "lck.h"
#include "lck_proto.h"
#include "val.h"
#include "sbm.h"
#include "Connect.h"
#include "PStatement.h"
#include "RSet.h"
#include "tra.h"
#include "Sync.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


Relation::Relation(Database *dbb, int id)
{
	rel_id = id;
	rel_database = dbb;
	rel_flags = 0;
	rel_current_fmt = 0;
	rel_length = 0;
	rel_current_format = NULL;
	//rel_pages = NULL;
	//rel_fields = NULL;
	rel_view_rse = NULL;
	rel_view_contexts = NULL;
	rel_file = NULL;
	rel_index_root = 0;
	rel_data_pages = 0;
	rel_gc_rec = NULL;
	rel_gc_bitmap = NULL;
	rel_slot_space = 0;
	rel_data_space = 0;
	useCount = 0;
	rel_sweep_count = 0;
	rel_scan_count = 0;
	rel_scan_count = 0;
	rel_dbkey_length = 0;
	rel_existence_lock = NULL;
	rel_interest_lock = NULL;
	rel_record_locking = NULL;
	rel_explicit_locks = 0;
	rel_read_locks = 0;
	rel_write_locks = 0;
	rel_lock_total = 0;
	rel_security_name = NULL;
	//rel_formats = NULL;
	
	rel_index_locks = NULL;
	rel_index_blocks = NULL;
	rel_pre_erase = NULL;
	rel_post_erase = NULL;
	rel_pre_modify = NULL;
	rel_post_modify = NULL;
	rel_pre_store = NULL;
	rel_post_store = NULL;
	
	dsqlRelation = NULL;
	junk = NULL;
}

Relation::~Relation()
{
	if (dsqlRelation)
		delete dsqlRelation;

	int n;
	
	for (n = 0; n < rel_fields.size(); ++n)
		delete rel_fields[n];

	for (n = 0; n < rel_formats.size(); ++n)
		delete rel_formats[n];
			
	for (Field *field; field = junk;)
		{
		junk = field->fld_junk;
		delete field;
		}
		
	delete rel_view_rse;
	delete rel_gc_bitmap;
	
	for (vcx *context; context = rel_view_contexts;)
		{
		rel_view_contexts = context->vcx_next;
		delete context;
		}
}

void Relation::scan(tdbb *tdbb)
{
	MET_scan_relation (tdbb, this);
}


prim::prim(void)
{
	prim_reference_ids = NULL;
	prim_relations = NULL;
	prim_indexes = NULL;
}

frgn::frgn(void)
{
	frgn_reference_ids = NULL;
	frgn_relations = NULL;
	frgn_indexes = NULL;
}

void Relation::getTypeInformation(tdbb* tdbb)
{
	MET_getTypeInformation (tdbb, this);
	rel_flags |= REL_has_type_info;
}

int Relation::getPrimaryKey(tdbb* tdbb, int maxFields, Field** fields)
{
	return MET_get_primary_key (tdbb, this, maxFields, fields);
}

Field* Relation::findField(tdbb* tdbb, const char* fieldName)
{
	if (!(rel_flags & REL_scanned))
		MET_scan_relation (tdbb, this);
	
	/***	
	if (rel_fields) 
		{
		int length = strlen(fieldName);
		for (vec::iterator fieldIter = rel_fields->begin(), end = rel_fields->end();  fieldIter < end; fieldIter++)
			if (*fieldIter) 
				{
				Field* field = (Field*) *fieldIter;
				if (field->fld_length == length && field->fld_name == fieldName)
					return field;
				}
		}
	***/

	int length = strlen(fieldName);
	
	for (int n = 0; n < rel_fields.size(); ++n)
		{
		Field *field = rel_fields[n];
		
		if (field && field->fld_length == length && field->fld_name == fieldName)
			return field;
		}
	
	return NULL;
}

Field* Relation::getField(tdbb* tdbb, const char* fieldName)
{
	Field *field = findField (tdbb, fieldName);

	if (!field)
		ERR_post (isc_fldnotdef, isc_arg_string, fieldName, isc_arg_string, (const char*) rel_name,
				  isc_arg_end);
	
	return field;
}

int Relation::blockingAst(void* astObject)
{
	return ((Relation*) astObject)->blockingAst();
}

int Relation::blockingAst(void)
{
	struct tdbb thd_context, *tdbb;

	/* Since this routine will be called asynchronously, we must establish
	   a thread context. */

	SET_THREAD_DATA;

	tdbb->tdbb_database = rel_existence_lock->lck_dbb;
	tdbb->tdbb_attachment = rel_existence_lock->lck_attachment;
	tdbb->tdbb_request = NULL;
	tdbb->tdbb_transaction = NULL;
	tdbb->tdbb_default = NULL;

	if (useCount)
		rel_flags |= REL_blocking;
	else 
		{
		rel_flags &= ~REL_blocking;
		rel_flags |= (REL_check_existence | REL_check_partners);
		if (rel_existence_lock)
			LCK_release(rel_existence_lock);
		}

	/* Restore the prior thread context */

	RESTORE_THREAD_DATA;
	
	return 0; 
}

void Relation::createExistenceLock(tdbb* tdbb)
{
	LCK lock = rel_existence_lock = FB_NEW_RPT(*rel_database->dbb_permanent, 0) lck;
	lock->lck_parent = rel_database->dbb_lock;
	lock->lck_dbb = rel_database;
	lock->lck_key.lck_long = rel_id;
	lock->lck_length = sizeof(lock->lck_key.lck_long);
	lock->lck_type = LCK_rel_exist;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, LCK_rel_exist);
	lock->lck_object = (BLK) this;
	lock->lck_ast = blockingAst;
}

void Relation::fetchFields(Connection* connection)
{
	rel_fields.checkSize(10, 10);
	
	/***
	if (!rel_fields)
		rel_fields = vec::newVector(*rel_database->dbb_permanent, 10);
	***/
	
	PStatement statement = connection->prepareStatement (
		"select"
		"   rfr.rdb$field_name,"
		"   rfr.rdb$field_position "
		"from "
		"   rdb$relation_fields rfr join"
		"   rdb$fields fld on"
		"      rfr.rdb$field_source = fld.rdb$field_name "
		"where"
		"    rdb$relation_name = ?");
		
	statement->setString(1, rel_name);
	RSet resultSet = statement->executeQuery();
	
	while (resultSet->next())
		{
		int seq = resultSet->getInt(2);
		const char *fieldName = resultSet->getString(1);
		const char *p;
		
		for (p = fieldName; *p && *p != ' '; ++p)
			;
			
		Field *field = new Field (JString(fieldName, p - fieldName));
		rel_fields.checkSize(seq, seq + 10);
		delete rel_fields[seq];
		rel_fields [seq] = field;
		}
	
		
		
}

void Relation::fetchFields(Transaction* transaction)
{
	Connect connection = transaction->getConnection();
	fetchFields (connection);
}

void Relation::dropField(tdbb* tdbb, const char* fieldName)
{
	Sync sync(&syncObject, "Relation::dropField");
	sync.lock(Exclusive);
	Field *field = getField(tdbb, fieldName);
	
	if (field)
		{
		rel_fields [field->fld_id] = NULL;
		field->fld_junk = junk;
		junk = field;
		
		if (dsqlRelation && field->dsqlField)
			dsqlRelation->dropField(field->dsqlField);
		}
}

Field* Relation::addField(int id, const char* name)
{
	Sync sync(&syncObject, "Relation::addField");
	sync.lock(Exclusive);
	Field *field = new Field(name);
	rel_fields.checkSize(id, id + 1);
	field->fld_id = id;
	rel_fields[id] = field;
	
	if (dsqlRelation)
		field->dsqlField = dsqlRelation->addField(field->fld_id, field->fld_name);
		
	return field;
}

int Relation::incrementUseCount(void)
{
	return INTERLOCKED_INCREMENT(useCount);
}

int Relation::decrementUseCount(void)
{
	return INTERLOCKED_DECREMENT(useCount);
}

Field* Relation::findField(int id)
{
	if (id >= rel_fields.size())
		return NULL;
	
	return rel_fields[id];
}

fmt* Relation::getFormat(tdbb* tdbb, int formatVersion)
{
	fmt *format;
	Sync sync(&syncFormats, "Relation::getFormat");
	sync.lock(Shared);
	
	if (formatVersion < rel_formats.size() && (format = rel_formats [formatVersion]))
		return format;
	
	sync.unlock();
	format = MET_format(tdbb, this, formatVersion);
	sync.lock(Exclusive);
	rel_formats.checkSize(formatVersion, formatVersion+1);
	
	if (rel_formats[formatVersion])
		{
		delete format;
		return rel_formats[formatVersion];
		}
	
	rel_formats[formatVersion] = format;
	
	return format;
}

fmt* Relation::getCurrentFormat(tdbb* tdbb)
{
	if (!rel_current_format)
		rel_current_format = getFormat(tdbb, rel_current_fmt);
	
	return rel_current_format;
}

void Relation::setFormat(fmt* format)
{
	Sync sync(&syncFormats, "Relation::setFormat");
	sync.lock(Exclusive);
	int formatVersion = format->fmt_version;
	rel_formats.checkSize(formatVersion, formatVersion+1);
	rel_formats[formatVersion] = format;
}

void Relation::scanRelation(tdbb *tdbb, int csb_flags)
{
	if ((!(rel_flags & REL_scanned) || (rel_flags & REL_being_scanned)) &&
		((rel_flags & REL_force_scan) || !(csb_flags & csb_internal)))
		{
		rel_flags &= ~REL_force_scan;
		MET_scan_relation(tdbb, this);
		}
	else if (rel_flags & REL_sys_triggers)
		MET_parse_sys_trigger(tdbb, this);

}
