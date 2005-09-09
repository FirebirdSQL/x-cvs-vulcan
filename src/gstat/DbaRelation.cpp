/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbaRelation.cpp
 *	DESCRIPTION:	Database analysis tool
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
 * 2001.08.07 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "DbaRelation.h"
#include "DbaIndex.h"
#include "DbaData.h"


DbaRelation::DbaRelation(void)
{
	rel_indexes = NULL;
	rel_next = NULL;
	rel_index_root = 0;
	rel_pointer_page = 0;
	rel_slots = 0;
	rel_data_pages = 0;
	rel_records = 0;
	rel_record_space = 0;
	rel_versions = 0;
	rel_version_space = 0;
	rel_max_versions = 0;
	memset(rel_fill_distribution, 0, sizeof(rel_fill_distribution[BUCKETS]));
	rel_total_space = 0;
	rel_id = 0;
}

DbaRelation::~DbaRelation(void)
{
	for (DbaIndex *index; index = rel_indexes;)
		{
		rel_indexes = index->idx_next;
		delete index;
		}
}
