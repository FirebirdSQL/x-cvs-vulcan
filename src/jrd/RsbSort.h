/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbSort.h
 *	DESCRIPTION:	Record source block definitions
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
 * Refactored July 1, 2005 by James A. Starkey
 */

#ifndef JRD_RSE_SORT_H
#define JRD_RSE_SORT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"
#include "dsc.h"

struct SortContext;

// Sort map block

struct smb_repeat {
	DSC smb_desc;				// relative descriptor
	USHORT smb_flag_offset;		// offset of missing flag
	USHORT smb_stream;			// stream for field id
	SSHORT smb_field_id;		// id for field (-1 if dbkey)
	struct jrd_nod *smb_node;	// expression node
};

class SortMap : public pool_alloc_rpt<smb_repeat, type_smb>
{
public:
	USHORT smb_keys;			// number of keys
	USHORT smb_count;			// total number of fields
	USHORT smb_length;			// sort record length
	USHORT smb_key_length;		// key length in longwords
	struct SortKeyDef* smb_key_desc;	// address of key descriptors
	USHORT smb_flags;			// misc sort flags
    smb_repeat smb_rpt[1];
};

// values for smb_field_id

const SSHORT SMB_DBKEY = -1;	// dbkey value
const SSHORT SMB_TRANS_ID = -2;	// transaction id of record

// bits for the smb_flags field

const USHORT SMB_project = 1;	// sort is really a project
const USHORT SMB_tag = 2;		// beast is a tag sort


struct irsb_sort {
	ULONG		  irsb_flags;
	SortContext* irsb_sort_handle;
};

typedef irsb_sort *IRSB_SORT;


class RsbSort : public RecordSource
{
public:
	RsbSort(CompilerScratch *csb, RecordSource *source, SortMap *sortMap);
	virtual ~RsbSort(void);
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen);
	virtual void close(Request* request);
	static bool reject(const UCHAR* record_a, const UCHAR* record_b, void* user_arg);
	virtual void pushRecords(Request* request);
	virtual void popRecords(Request* request);
	
	SortMap		*map;
};

#endif

