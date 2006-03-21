/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		DbaData.epp
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

#ifndef _DBA_DATA_H
#define _DBA_DATA_H

#include <stdio.h>
#include "../jrd/msg_encode.h"

#define GSTAT_CODE(n)	ENCODE_ISC_MSG(1, GSTAT_MSG_FAC)

const USHORT GSTAT_MSG_FAC	= 21;
const SSHORT BUCKETS	= 5;

class DbaFile;
class DbaRelation;
class DbaOpenFile;
class Service;

struct pag;

class DbaData
{
public:
	DbaData(); //: ThreadData(tddDBA) 
	~DbaData();
	
	//UCHAR		*dba_env;
	DbaFile*	files;
	DbaRelation*	relations;
	SSHORT		page_size;
	SLONG		page_number;
	pag*		buffer1;
	pag*		buffer2;
	pag*		global_buffer;
	int			exit_code;
	
#ifdef SERVICE_THREAD
	Service		*sw_outfile;
	//dba_mem	*head_of_mem_list;
	//DbaOpenFile	*head_of_files_list;
	Service		*dba_service_blk;
#else
	FILE*	sw_outfile;
#endif

	ISC_STATUS *dba_status;
	ISC_STATUS_ARRAY dba_status_vector;
	void closeFiles(void);
	DbaFile* addFile(const char* fileName);
	pag* read(ULONG pageNumber);
};

#endif
