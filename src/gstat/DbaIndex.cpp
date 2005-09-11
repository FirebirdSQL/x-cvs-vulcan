/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbaIndex.cpp
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
#include "DbaIndex.h"


DbaIndex::DbaIndex(void)
{
	idx_id = 0;
	idx_depth = 0;
	idx_leaf_buckets = 0;
	idx_total_duplicates = 0;
	idx_max_duplicates = 0;
	idx_nodes = 0;
	idx_data_length = 0;
	memset(idx_fill_distribution, 0, sizeof(idx_fill_distribution));
}

DbaIndex::~DbaIndex(void)
{
}
