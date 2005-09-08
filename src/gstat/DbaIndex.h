/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbaFile.epp
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

#ifndef _DBA_INDEX_H
#define _DBA_INDEX_H

#include "JString.h"
#include "DbaData.h"

class DbaIndex
{
public:
	DbaIndex(void);
	~DbaIndex(void);
	DbaIndex*	idx_next;
	SSHORT		idx_id;
	SSHORT		idx_depth;
	SLONG		idx_leaf_buckets;
	SLONG		idx_total_duplicates;
	SLONG		idx_max_duplicates;
	SLONG		idx_nodes;
	SLONG		idx_data_length;
	SLONG		idx_fill_distribution[BUCKETS];
	//SCHAR		idx_name[32];
	JString		idx_name;
};

#endif
