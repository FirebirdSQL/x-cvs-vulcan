/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dba.epp
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
#include "DbaData.h"
#include "DbaRelation.h"
#include "DbaFile.h"
//#include "DbaOpenFile.h"
#include "../jrd/ods.h"

DbaData::DbaData(void)
{
	files = NULL;
	relations = NULL;
	page_size = 0;
	page_number = 0;
	buffer1 = NULL;
	buffer2 = NULL;
	global_buffer = NULL;
	exit_code = 0;
	
#ifdef SERVICE_THREAD
	sw_outfile = 0;
	//head_of_mem_list = 0;
	head_of_files_list = NULL;
	dba_service_blk = 0;
#else
	sw_outfile = 0;
#endif

	memset(dba_status_vector, 0, sizeof (dba_status_vector));
	dba_status = dba_status_vector;
}

DbaData::~DbaData(void)
{
	closeFiles();
	delete [] buffer1;
	delete [] buffer2;
	delete [] global_buffer;
	
	for (DbaRelation *relation; relation = relations;)
		{
		relations = relation->rel_next;
		delete relation;
		}
	
	for (DbaFile *file; file = files;)
		{
		files = file->fil_next;
		delete file;
		}
	
#ifdef SERVICE_THREAD
	for (DbaOpenFile *openFile = head_of_files_list; openFile;)
		{
		head_of_files_list = openFile->open_files_next;
		delete openFile;
		}
#endif
}

void DbaData::closeFiles(void)
{
	for (DbaFile *file = files; file; file = file->fil_next)
		file->close();
}

DbaFile* DbaData::addFile(const char* fileName)
{
	DbaFile* file = new DbaFile(fileName);
	
	if (files) 
		{
		for (file = files; file->fil_next; file = file->fil_next)
			;
			
		file->fil_next = file;
		file->fil_next->fil_min_page = file->fil_max_page + 1;
		file = file->fil_next;
		}
	else 
		{
		file = files = file;
		file->fil_min_page = 0L;
		}

	file->fil_next = NULL;
	file->fil_fudge = 0;
	file->fil_max_page = 0L;
	
	return file;
}

pag* DbaData::read(ULONG pageNumber)
{
	if (pageNumber == page_number)
		return global_buffer;

	page_number = pageNumber;
	DbaFile* file;
	
	for (file = files; pageNumber > file->fil_max_page && file->fil_next;) 
		 file = file->fil_next;

	pageNumber -= file->fil_min_page - file->fil_fudge;
	UINT64 liOffset = (UINT64) pageNumber * page_size;
	file->read(liOffset, page_size, global_buffer);
	
	return global_buffer;
}
